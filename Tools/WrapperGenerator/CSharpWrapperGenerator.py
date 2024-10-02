from copy import deepcopy
from pathlib import Path
from shutil import rmtree
from typing import Dict, List, Union, Any

import os
import subprocess
import chevron

from Config import config
from MetadataTypes import (
    EnumMetadata,
    StructMetadata,
    FunctionMetadata,
    ClassMetadata,
    TemplateMetadata,
    TypeMetadata,
    InterfaceMetadata,
    ParameterMetadata,
)
from Parser import read_whole_file, error_in_file, warning_in_file


class CSharpWrapperGenerator:
    """Convert and output the parsed C++ types into corresponding C# types.
    
    C++ types are translated into the equivalent C# types.

    The callback-based methods in the C++ API are converted into methods
    returning Task<T> objects for use with native C# `async` methods.
    """
    __TEMPLATE_DIRECTORY = config["template_directory"] + "CSharp/"
    __PARTIALS_DIRECTORY = __TEMPLATE_DIRECTORY + "Partials/"
    __OUTPUT_DIRECTORY = config["output_directory"] + "CSharp/"
    __BASE_NAMESPACE = "Csp"

    __NAMESPACE_TRANSLATIONS = {
        "csp": __BASE_NAMESPACE,
        "common": "Common",
        "memory": "Memory",
        "multiplayer": "Multiplayer",
        "systems": "Systems",
        "web": "Web",
    }

    enums: Dict[str, EnumMetadata]
    structs: Dict[str, StructMetadata]
    functions: Dict[str, FunctionMetadata]
    classes: Dict[str, ClassMetadata]
    templates: Dict[str, TemplateMetadata]
    interfaces: Dict[str, InterfaceMetadata]

    def __translate_namespace(self, obj: Any) -> None:
        if obj.namespace is None:
            obj.translated_namespace = None
            return

        namespaces = obj.namespace.split("::")

        for i in range(0, min(2, len(namespaces))):
            if namespaces[i] in self.__NAMESPACE_TRANSLATIONS:
                namespaces[i] = self.__NAMESPACE_TRANSLATIONS[namespaces[i]]

        obj.translated_namespace = ".".join(namespaces)

    def __translate_enum_base(self, obj: EnumMetadata) -> None:
        t = obj.base

        if t == "uint8_t" or t == "unsigned char":
            obj.base = "byte"
        elif t == "uint16_t" or t == "unsigned short":
            obj.base = "ushort"

    def __translate_type(self, obj: TypeMetadata) -> None:
        self.__translate_namespace(obj)

        if obj.is_template:
            assert obj.template_arguments is not None

            for ta in obj.template_arguments:
                self.__translate_type(ta.type)

        t = obj.name

        if not hasattr(obj, "is_void_pointer"):
            obj.is_void_pointer = False

        match t:
            case "int8_t":
                obj.name = "sbyte"
            case "uint8_t" | "unsigned char":
                obj.name = "byte"
            case "int16_t":
                obj.name = "short"
            case "uint16_t":
                obj.name = "ushort"
            case "int32_t" | "long":
                obj.name = "int"
            case "uint32_t" | "unsigned int" | "unsigned long":
                obj.name = "uint"
            case "int64_t" | "long long" | "long int":
                obj.name = "long"
            case "uint64_t" | "unsigned long long" | "unsigned long int":
                obj.name = "ulong"
            case "size_t":
                # Assume 64-bit only for now
                obj.name = "ulong"
            case "String":
                obj.name = "string"
                obj.namespace = None
                obj.translated_namespace = None
                obj.is_pointer = False
                obj.is_reference = False
                obj.is_pointer_or_reference = False
            case "void" | "char" if obj.is_pointer:
                obj.name = "IntPtr"
                obj.is_pointer = False
                obj.is_reference = False
                obj.is_pointer_or_reference = False
                obj.is_void_pointer = True
                obj.translated_namespace = None

    def __translate_comments(self, comments: List[str]) -> None:
        """Rewrite a list of comments from Javadoc-style to C# XML style"""
        if comments is None:
            return

        for i, comment in enumerate(comments):
            comment = comment.replace("<", "&lt;").replace(">", "&gt;")

            if comment[0] != "@":
                comments[i] = f"<remarks>{comment}</remarks>"
                continue

            index = comment.find(" ")
            tag = comment[:index]
            content = comment[index + 1:]

            if tag == "@brief":
                comments[i] = f"<summary>{content}</summary>"
            elif tag == "@return":
                index = content.find(":")

                while content[index + 1] == ":":
                    index = content.find(":", index + 2)

                content = content[index + 1:].lstrip()

                if content[0].islower():
                    content = content.capitalize()

                comments[i] = f"<returns>{content}</returns>"
            elif tag == "@param":
                index = content.find(" ")
                var_name = content[:index]
                content = content[index + 1:].lstrip()

                index = content.find(":")

                while content[index + 1] == ":":
                    index = content.find(":", index + 2)

                content = content[index + 1:].lstrip()

                if content[0].islower():
                    content = content.capitalize()

                comments[i] = f'<param name="{var_name}">{content}</param>'
            elif tag == "@note":
                comments[i] = f"<remarks>{content}</remarks>"

    def __class_derives_from(
        self,
        obj: ClassMetadata,
        base_namespace: str,
        base_name: str,
        classes: Dict[str, ClassMetadata],
    ) -> bool:
        if obj.base is None:
            return False

        if obj.base.namespace == base_namespace and obj.base.name == base_name:
            return True

        full_type_name = f"{obj.base.namespace}::{obj.base.name}"

        if full_type_name not in classes:
            return False

        base_class = classes[full_type_name]

        return self.__class_derives_from(base_class, base_namespace, base_name, classes)

    def __get_file_output_directory(
        self,
        obj: Union[
            EnumMetadata,
            StructMetadata,
            InterfaceMetadata,
            ClassMetadata,
            TemplateMetadata,
        ],
    ):
        header_file: str

        if isinstance(obj, TemplateMetadata):
            header_file = obj.definition.header_file
        else:
            header_file = obj.header_file

        out_path = header_file.split("/")
        out_path = out_path[1:-1]

        return "/".join(out_path)

    def __render_named_object(self, named_object, enum_template) -> None:
        """Given an object with a "name" attribute, render it to the corresponding .cs file."""
        subdir = self.__get_file_output_directory(named_object)
        if named_object.surrounding_types:
            for st in named_object.surrounding_types:
                subdir = f"{subdir}/{st}"

        Path(self.__OUTPUT_DIRECTORY + subdir).mkdir(parents=True, exist_ok=True)

        file_path = f"{self.__OUTPUT_DIRECTORY}{subdir}/{named_object.name}.cs"
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(
                chevron.render(
                    enum_template,
                    {"data": named_object, "extra_data": config},
                    self.__PARTIALS_DIRECTORY,
                    warn=True,
                )
            )

    def generate(
        self,
        enums: Dict[str, EnumMetadata],
        structs: Dict[str, StructMetadata],
        functions: Dict[str, FunctionMetadata],
        classes: Dict[str, ClassMetadata],
        templates: Dict[str, TemplateMetadata],
        interfaces: Dict[str, InterfaceMetadata],
    ) -> None:
        """Given the dictionaries passed in from the Parser, output the corresponding C# source."""

        # Deepcopy all metadata so we don't modify the original data for any wrapper generator classes that get called after this one
        self.enums = deepcopy(enums)
        self.structs = deepcopy(structs)
        self.functions = deepcopy(functions)
        self.classes = deepcopy(classes)
        self.templates = deepcopy(templates)
        self.interfaces = deepcopy(interfaces)

        out_path = Path(self.__OUTPUT_DIRECTORY)

        # Remove the output directory first to delete all previously generated code
        if out_path.exists():
            rmtree(out_path)

        # Recreate the output directory
        out_path.mkdir(parents=True, exist_ok=True)

        enum_template = read_whole_file(self.__TEMPLATE_DIRECTORY + "Enum.mustache")
        struct_template = read_whole_file(self.__TEMPLATE_DIRECTORY + "Struct.mustache")
        global_functions_template = read_whole_file(
            self.__TEMPLATE_DIRECTORY + "GlobalFunctions.mustache"
        )
        class_template = read_whole_file(self.__TEMPLATE_DIRECTORY + "Class.mustache")
        interface_template = read_whole_file(
            self.__TEMPLATE_DIRECTORY + "Interface.mustache"
        )
        templateclass_template = read_whole_file(
            self.__TEMPLATE_DIRECTORY + "Template.mustache"
        )

        self.__translate_functions(self.functions)

        self.__render_enums(enum_template)
        self.__render_structs(struct_template)
        self.__render_global_functions(global_functions_template)
        self.__render_interfaces(interface_template)
        self.__render_classes(class_template)
        self.__render_templates(templateclass_template)

        self.format_output()

    def format_output(self):
        """Format output with CSharpier"""
        script_directory = os.path.dirname(os.path.realpath(__file__))
        # TODO(OB-3780): This will fail with a warning on Mac since you can't run a .NET executable directly
        subprocess.run(
            f'"{script_directory}\\Formatters\\CSharpier\\dotnet-csharpier.exe" "{self.__OUTPUT_DIRECTORY}"',
            shell=True,
            check=False,  # When fixed, change this to check=True
        )

    def __translate_functions(self, functions) -> None:
        """Translate the global functions by rewriting the parameter and return types."""
        for f in functions.values():
            self.__translate_comments(f.doc_comments)

            if f.return_type is not None:
                self.__translate_type(f.return_type)

            if f.parameters is not None:
                for p in f.parameters:
                    self.__translate_type(p.type)

                    if p.type.is_function_signature:
                        assert p.type.function_signature is not None

                        if p.type.function_signature.parameters is not None:
                            for fp in p.type.function_signature.parameters:
                                self.__translate_type(fp.type)

    def __render_enums(self, enum_template) -> None:
        """Render the parsed enums into corresponding C# files."""
        for e in self.enums.values():
            e.surrounding_types = None

            if e.is_nested_type:
                e.surrounding_types = e.namespace[e.namespace.find(
                    "::") + 2:].split("::")
                e.namespace = e.namespace[: e.namespace.find("::")]

            if e.doc_comments is not None:
                self.__translate_comments(e.doc_comments)

            self.__translate_namespace(e)
            self.__translate_enum_base(e)

            for f in e.fields:
                if f.doc_comments is not None:
                    self.__translate_comments(f.doc_comments)

            self.__render_named_object(e, enum_template)

    def __render_structs(self, struct_template) -> None:
        """Render the parsed structs into corresponding C# files."""
        for s in self.structs.values():
            s.surrounding_types = None

            if s.is_nested_type:
                s.surrounding_types = s.namespace[s.namespace.find(
                    "::") + 2:].split("::")
                s.namespace = s.namespace[: s.namespace.find("::")]

            self.__translate_comments(s.doc_comments)
            self.__translate_namespace(s)
            self.__render_named_object(s, struct_template)

    def __render_global_functions(self, global_functions_template) -> None:
        """Render the global functions into the static global function class."""
        file_path = f"{self.__OUTPUT_DIRECTORY}Csp.cs"
        with open(file_path, "w", encoding="utf-8") as f:
            print(
                chevron.render(
                    global_functions_template,
                    {"data": list(self.functions.values()),
                     "extra_data": config},
                    self.__PARTIALS_DIRECTORY,
                    warn=True,
                ),
                file=f,
            )

    def __render_interfaces(self, interface_template) -> None:
        """Rewrite and render the interfaces into corresponding C# files."""
        for i in self.interfaces.values():
            i.surrounding_types = None

            if i.is_nested_type:
                i.surrounding_types = i.namespace[i.namespace.find(
                    "::") + 2:].split("::")
                i.namespace = i.namespace[: i.namespace.find("::")]

            self.__translate_comments(i.doc_comments)
            self.__translate_namespace(i)

            delegates = []
            events = []

            self.__rewrite_methods(i.methods, delegates, events, True)

            i.delegates = delegates
            i.events = events
            i.has_events = len(events) > 0

            self.__render_named_object(i, interface_template)

    def __render_classes(self, class_template) -> None:
        """Rewrite and render the classes into corresponding C# files."""
        for c in self.classes.values():
            c.surrounding_types = None

            if c.is_nested_type:
                c.surrounding_types = c.namespace[c.namespace.find(
                    "::") + 2:].split("::")
                c.namespace = c.namespace[: c.namespace.find("::")]

            self.__translate_comments(c.doc_comments)
            self.__translate_namespace(c)

            if c.base:
                self.__translate_namespace(c.base)

            delegates = []
            events = []

            self.__rewrite_fields(c)

            self.__rewrite_methods(c.methods, delegates, events)

            c.delegates = delegates
            c.events = events
            c.has_events = len(events) > 0

            self.__render_named_object(c, class_template)

    def __rewrite_fields(self, c: ClassMetadata) -> None:
        """Rewrite the fields of a class to the corresponding C# type names."""
        for f in c.fields:
            self.__translate_type(f.type)

            if f.type.is_template:
                assert f.type.template_arguments

                for ta in f.type.template_arguments:
                    self.__translate_type(ta.type)

    def __rewrite_methods(
        self,
        methods: List[FunctionMetadata],
        delegates: List[Dict],
        events: List[Dict],
        is_interface=False,
    ) -> None:
        """Rewrite the methods of a class or interface.
        
        If the methods have callback parameters they will be wrapped in logic to return a Task<> of the corresponding callback type.
        Set__Callback functions will be converted into On__ events.
        """
        for m in methods:
            self.__translate_comments(m.doc_comments)

            if m.return_type:
                self.__translate_type(m.return_type)

                if m.return_type.is_template:
                    assert m.return_type.template_arguments

                    for ta in m.return_type.template_arguments:
                        self.__translate_type(ta.type)

            m.is_task = m.is_async_result or m.is_async_result_with_progress

            self.__rewrite_task_doc_comments(m)

            if m.parameters:
                self.__rewrite_method_parameters(m)
                self.__rewrite_callback_parameters(m, delegates, events, is_interface)

    def __rewrite_method_parameters(self, method: FunctionMetadata) -> None:
        """Rewrite C++ types to C# types in a method definition."""
        for param in method.parameters:
            self.__translate_type(param.type)

            if param.type.is_template:
                assert param.type.template_arguments is not None

                for ta in param.type.template_arguments:
                    self.__translate_type(ta.type)

    def __rewrite_callback_parameters(
        self,
        method: FunctionMetadata,
        delegates: List[Dict],
        events: List[Dict],
        is_interface: bool,
    ) -> None:
        """Finds callback parameters in a method and creates a custom delegate type"""
        # TODO: Rather than generate a unique delegate type per method, it would be more efficient to
        # translate the shared *Callback types into shared delegate definitions and reuse them here.

        is_regular_method = not (
            method.is_async_result or method.is_async_result_with_progress or method.is_event
        )

        if is_interface:
            if not is_regular_method:
                error_in_file(
                    method.header_file,
                    method.start_line,
                    r"Method '{m.name}' is not a regular function. Interfaces cannot currently contain asynchronous methods",
                )
                return

        # Find parameters that take function signatures and ensure that there's only one of them:
        # If there's more than one, we won't be able to create an async method from it later.
        callback_params = [p for p in method.parameters if p.type.is_function_signature]
        if not callback_params:
            return

        if len(callback_params) > 1:
            error_in_file(
                method.header_file,
                method.start_line,
                r"Method '{m.name}' contains more than one callback parameter so cannot be converted to an async function"
            )
            return

        # Create the custom delegate based on the function signature of the callback
        p = callback_params[0]
        function_signature = p.type.function_signature
        method.results = function_signature.parameters
        method.has_results = bool(method.results)
        method.has_multiple_results = len(method.results) > 1

        param_name = p.name[0].upper() + p.name[1:]

        if function_signature.parameters:
            for dp in function_signature.parameters:
                self.__translate_type(dp.type)

                full_type_name = f"{dp.type.namespace}::{dp.type.name}"
                dp.type.is_result_base = (
                    full_type_name in self.classes
                    and self.__class_derives_from(
                        self.classes[full_type_name],
                        "csp::systems",
                        "ResultBase",
                        self.classes,
                    )
                )

        delegate = {
            "name": f"{method.name}{param_name}Delegate",
            "method_name": method.name,
            "return_type": function_signature.return_type,
            "parameters": deepcopy(function_signature.parameters),
            "has_parameters": bool(function_signature.parameters),
            "has_progress": method.is_async_result_with_progress,
            "include_managed": is_regular_method,
        }

        delegates.append(delegate)
        method.delegate = delegate
        p.delegate = delegate

        # Generate an event for functions decorated with the event macro
        if method.is_event:
            self.__rewrite_event(method, p, delegate, events)

        # For any methods that will be generating a callback, remove the callback
        # parameter from the method signature
        if not is_regular_method:
            method.parameters.remove(p)
            if len(method.parameters) > 0:
                method.parameters[-1].is_last = True

    def __rewrite_event(
        self,
        method: FunctionMetadata,
        param: ParameterMetadata,
        delegate: Dict,
        events: List[Dict],
    ) -> None:
        """Create a C# event field for SetCallback methods"""
        event_name = ""

        assert method.name

        # Rename `Set____Callback` to `On____`
        # e.g. `SetDisconnectionCallback` becomes `OnDisconnection`
        # Note: This violates C# naming conventions, which would name the event `Disconnected`
        if method.name.startswith("Set") and method.name.endswith("Callback"):
            event_name = f"On{method.name[len('Set'):-len('Callback')]}"
        else:
            warning_in_file(
                method.header_file,
                method.start_line,
                "Event functions should follow the naming pattern 'SetXCallback'.",
            )
            event_name = method.name

        event = {
            "name": event_name,
            "class_name": method.parent_class.name,
            "method_name": method.name,
            "unique_method_name": method.unique_name,
            "parameters": deepcopy(param.type.function_signature.parameters),
            "has_parameters": bool(param.type.function_signature.parameters),
            "has_multiple_parameters": (
                param.type.function_signature.parameters
                and len(param.type.function_signature.parameters) > 1
            ),
            "delegate": delegate,
        }

        events.append(event)
        method.event = event

    def __rewrite_task_doc_comments(self, method: FunctionMetadata):
        if method.is_task and method.doc_comments:
            assert method.parameters is not None

            param = method.parameters[-1]
            method.doc_comments = method.doc_comments[:-1]

            assert param.type.function_signature is not None

            if len(param.type.function_signature.doc_comments) > 0:
                comment = param.type.function_signature.doc_comments[-1]
                comment = comment.replace("<", "&lt;").replace(">", "&gt;")

                comment_index = comment.find(" ")
                tag = comment[:comment_index]

                if tag != "@param":
                    error_in_file(
                        method.header_file or "", -1, "Error in comment: " + comment
                    )
                    error_in_file(
                        method.header_file or "",
                        -1,
                        "Last doc comment must describe callback parameter",
                    )

                content = comment[comment_index + 1:]
                comment_index = content.find(" ")
                # var_name = content[:comment_index]
                content = content[comment_index + 1:].lstrip()

                comment_index = content.find(":")

                while content[comment_index + 1] == ":":
                    comment_index = content.find(":", comment_index + 2)

                    # var_type = content[:comment_index]
                content = content[comment_index + 1:].lstrip()

                if content[0].islower():
                    content = content.capitalize()

                method.doc_comments.append(f"<returns>{content}</returns>")
            else:
                method.doc_comments.append("<returns>The result for the request</returns>")

    def __render_templates(self, templateclass_template) -> None:
        for t in self.templates.values():
            self.__translate_namespace(t.definition)

            for m in t.definition.methods:
                if m.return_type is not None:
                    self.__translate_type(m.return_type)

                if m.parameters is not None:
                    for p in m.parameters:
                        self.__translate_type(p.type)

            subdir = self.__get_file_output_directory(t)
            Path(self.__OUTPUT_DIRECTORY + subdir).mkdir(parents=True, exist_ok=True)

            path = f"{self.__OUTPUT_DIRECTORY}{subdir}/{t.definition.name}.cs"
            with open(path, "w", encoding="utf-8") as f:
                f.write(
                    chevron.render(
                        templateclass_template,
                        {"data": t, "extra_data": config},
                        self.__PARTIALS_DIRECTORY,
                        warn=True,
                    )
                )
