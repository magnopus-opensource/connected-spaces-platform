from copy import deepcopy
from pathlib import Path
from typing import Any, Dict, List

import subprocess
import shutil
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
)
from Parser import read_whole_file
from TypeScriptWrapperGenerator_Jinja2 import TypeScriptWrapperGenerator_Jinja2


class TypeScriptWrapperGenerator:
    __TEMPLATE_DIRECTORY = config["template_directory"] + "TypeScript/"
    __PARTIALS_DIRECTORY = __TEMPLATE_DIRECTORY + "Partials/"
    __OUTPUT_DIRECTORY = config["output_directory"] + "TypeScript/"

    NAMESPACE_TRANSLATIONS = {
        "csp": None,
        "common": "Common",
        "memory": "Memory",
        "multiplayer": "Multiplayer",
        "systems": "Systems",
        "web": "Web",
    }

    def __translate_namespace(self, obj: Any) -> None:
        if obj.namespace is None:
            setattr(obj, "translated_namespace", None)

            return

        namespaces = obj.namespace.split("::")

        for i in range(0, min(2, len(namespaces))):
            if namespaces[i] in self.NAMESPACE_TRANSLATIONS:
                namespaces[i] = self.NAMESPACE_TRANSLATIONS[namespaces[i]]

        setattr(obj, "translated_namespace", ".".join(filter(None, namespaces)))

    def __translate_type(self, type: TypeMetadata) -> None:
        if type is None:
            return

        self.__translate_namespace(type)

        if type.is_template and type.template_arguments is not None:
            for ta in type.template_arguments:
                self.__translate_type(ta.type)

        t = type.name

        setattr(type, "is_bool", False)
        setattr(type, "is_void_pointer", False)

        if not hasattr(type, "is_number"):
            setattr(type, "is_number", False)
            setattr(type, "is_float", False)
            setattr(type, "is_large", False)
            setattr(type, "is_small", False)
            setattr(type, "is_tiny", False)
            setattr(type, "is_signed", False)

        if t == "bool":
            type.name = "boolean"
            setattr(type, "is_bool", True)
        elif t == "uint64_t" or t == "unsigned long long" or t == "unsigned long long int":
            type.name = "bigint"
            setattr(type, "is_number", True)
            setattr(type, "is_large", True)
            setattr(type, "min_value", "Limits.UINT64_MIN")
            setattr(type, "max_value", "Limits.UINT64_MAX")
        elif (
            t == "int64_t"
            or t == "signed long long"
            or t == "signed long long int"
            or t == "long long"
            or t == "long long int"
        ):
            type.name = "bigint"
            setattr(type, "is_number", True)
            setattr(type, "is_large", True)
            setattr(type, "is_signed", True)
            setattr(type, "min_value", "Limits.INT64_MIN")
            setattr(type, "max_value", "Limits.INT64_MAX")
        elif (
            t == "size_t" or t == "uint32_t" or t == "unsigned int" or t == "unsigned long" or t == "unsigned long int"
        ):
            type.name = "number"
            setattr(type, "is_number", True)
            setattr(type, "min_value", "Limits.UINT32_MIN")
            setattr(type, "max_value", "Limits.UINT32_MAX")
        elif (
            t == "int32_t"
            or t == "signed int"
            or t == "signed long"
            or t == "signed long int"
            or t == "int"
            or t == "long"
            or t == "long int"
        ):
            type.name = "number"
            setattr(type, "is_number", True)
            setattr(type, "is_signed", True)
            setattr(type, "min_value", "Limits.INT32_MIN")
            setattr(type, "max_value", "Limits.INT32_MAX")
        elif t == "uint16_t" or t == "unsigned short" or t == "unsigned short int":
            type.name = "number"
            setattr(type, "is_number", True)
            setattr(type, "is_small", True)
            setattr(type, "min_value", "Limits.UINT16_MIN")
            setattr(type, "max_value", "Limits.UINT16_MAX")
        elif t == "int16_t" or t == "signed short" or t == "signed short int" or t == "short" or t == "short int":
            type.name = "number"
            setattr(type, "is_number", True)
            setattr(type, "is_small", True)
            setattr(type, "is_signed", True)
            setattr(type, "min_value", "Limits.INT16_MIN")
            setattr(type, "max_value", "Limits.INT16_MAX")
        elif t == "uint8_t" or t == "unsigned char":
            type.name = "number"
            setattr(type, "is_number", True)
            setattr(type, "is_tiny", True)
            setattr(type, "min_value", "Limits.UINT8_MIN")
            setattr(type, "max_value", "Limits.UINT8_MAX")
        elif t == "int8_t":
            type.name = "number"
            setattr(type, "is_number", True)
            setattr(type, "is_tiny", True)
            setattr(type, "is_signed", True)
            setattr(type, "min_value", "Limits.INT8_MIN")
            setattr(type, "max_value", "Limits.INT8_MAX")
        elif t == "float":
            type.name = "number"
            setattr(type, "is_number", True)
            setattr(type, "is_float", True)
            setattr(type, "min_value", "Limits.FLOAT_MIN")
            setattr(type, "max_value", "Limits.FLOAT_MAX")
        elif t == "double":
            type.name = "number"
            setattr(type, "is_number", True)
            setattr(type, "is_float", True)
            setattr(type, "is_large", True)
            setattr(type, "min_value", "Limits.DOUBLE_MIN")
            setattr(type, "max_value", "Limits.DOUBLE_MAX")
        elif t == "String":
            type.name = "string"
            type.namespace = None
            type.is_pointer = False
            type.is_reference = False
            type.is_pointer_or_reference = False
            setattr(type, "translated_namespace", None)
        elif t == "void" and type.is_pointer:
            setattr(type, "is_void_pointer", True)

        self.__translate_call_type(type)

    def __translate_call_type(self, type: TypeMetadata) -> None:
        if type.is_template or type.is_class or type.is_interface:
            setattr(type, "call_param_type", "number")
            setattr(type, "call_param_name_suffix", ".pointer")
        elif type.is_enum:
            setattr(type, "call_param_type", "number")
            setattr(type, "call_param_name_suffix", None)
        elif type.is_function_signature:
            setattr(type, "call_param_type", "number")
            setattr(type, "call_param_name_suffix", None)
        else:
            setattr(type, "call_param_type", type.name)
            setattr(type, "call_param_name_suffix", None)

    def __class_derives_from(
        self, obj: ClassMetadata, base_namespace: str, base_name: str, classes: Dict[str, ClassMetadata]
    ) -> bool:
        if obj.base is None:
            return False

        if obj.base.namespace == base_namespace and obj.base.name == base_name:
            return True

        full_type_name = f"{obj.base.namespace}::{obj.base.name}"

        if not full_type_name in classes:
            return False

        base_class = classes[full_type_name]

        return self.__class_derives_from(base_class, base_namespace, base_name, classes)

    def __translate_comments(self, comments: List[str]) -> None:
        if comments is None or len(comments) == 0:
            return

        for i in range(len(comments)):
            comment = comments[i]
            comment = comment.replace("<", "&lt;").replace(">", "&gt;")

            if comment[0] != "@":
                comments[i] = f" * {comment}  "
                continue

            index = comment.find(" ")
            tag = comment[:index]
            content = comment[index + 1 :]

            if tag == "@brief":
                comments[i] = f" * @description {content}  "
            elif tag == "@return":
                index = content.find(":")

                while content[index + 1] == ":":
                    index = content.find(":", index + 2)

                content = content[index + 1 :].lstrip()

                if content[0].islower():
                    content = content.capitalize()

                comments[i] = f" * @return {content}"
            elif tag == "@param":
                index = content.find(" ")
                var_name = content[:index]
                var_name = var_name[0].lower() + var_name[1:]
                content = content[index + 1 :].lstrip()

                index = content.find(":")

                while content[index + 1] == ":":
                    index = content.find(":", index + 2)

                content = content[index + 1 :].lstrip()

                if content[0].islower():
                    content = content.capitalize()

                comments[i] = f" * @param {var_name} - {content}"
            elif tag == "@note":
                comments[i] = f" * NOTE: {content}  "

        comments.insert(0, "/**")
        comments.append(" */")

    def generate(
        self,
        enums: Dict[str, EnumMetadata],
        structs: Dict[str, StructMetadata],
        functions: Dict[str, FunctionMetadata],
        classes: Dict[str, ClassMetadata],
        templates: Dict[str, TemplateMetadata],
        interfaces: Dict[str, InterfaceMetadata],
    ) -> None:
        # Deepcopy all metadata so we don't modify the original data for any wrapper generator classes that get called after this one
        enums = deepcopy(enums)
        structs = deepcopy(structs)
        functions = deepcopy(functions)
        classes = deepcopy(classes)
        templates = deepcopy(templates)
        interfaces = deepcopy(interfaces)

        for e in enums.values():
            surrounding_types = None

            if e.is_nested_type:
                surrounding_types = e.namespace[e.namespace.find("::") + 2 :].split("::")
                e.namespace = e.namespace[: e.namespace.find("::")]

            self.__translate_namespace(e)

            setattr(e, "surrounding_types", surrounding_types)

            if e.doc_comments is not None:
                self.__translate_comments(e.doc_comments)

        for i in interfaces.values():
            self.__translate_namespace(i)

            if i.doc_comments is not None:
                self.__translate_comments(i.doc_comments)

            for m in i.methods:
                if m.doc_comments is not None:
                    self.__translate_comments(m.doc_comments)

                # TODO: Move this to Parser
                if m.return_type is None:
                    m.return_type = TypeMetadata(namespace=None, name="void")
                    setattr(m.return_type, "translated_namespace", None)
                    setattr(m.return_type, "is_void", True)
                else:
                    setattr(m.return_type, "is_void", False)

                assert m.name is not None

                # Make first character lowercase to match camelCase standard used in Typescript
                m.name = m.name[0].lower() + m.name[1:]

                self.__translate_type(m.return_type)

                if m.return_type.is_template and m.return_type.template_arguments is not None:
                    for a in m.return_type.template_arguments:
                        self.__translate_type(a.type)

                setattr(m, "is_task", m.is_async_result or m.is_async_result_with_progress)

                if m.parameters is None:
                    continue

                for p in m.parameters:
                    self.__translate_type(p.type)

                    if p.type.is_template and p.type.template_arguments is not None:
                        for ta in p.type.template_arguments:
                            self.__translate_type(ta.type)

                    if p.type.is_function_signature and p.type.function_signature is not None:
                        # TODO: Move this to Parser
                        if p.type.function_signature.return_type is None:
                            p.type.function_signature.return_type = TypeMetadata(namespace=None, name="void")
                            setattr(p.type.function_signature.return_type, "translated_namespace", None)
                            setattr(p.type.function_signature.return_type, "is_void", True)
                        else:
                            setattr(p.type.function_signature.return_type, "is_void", False)

                        self.__translate_type(p.type.function_signature.return_type)

                        for pa in p.type.function_signature.parameters or []:
                            self.__translate_type(pa.type)
                            pa.name = pa.name[0].lower() + pa.name[1:]

                    if p.name.startswith("In") and p.name[2].isupper():
                        p.name = p.name[2:]

                    p.name = p.name[0].lower() + p.name[1:]

                    if not m.is_async_result and not m.is_async_result_with_progress:
                        continue

                    if not p.type.is_function_signature:
                        continue

                    assert p.type.function_signature is not None

                    setattr(m, "results", p.type.function_signature.parameters)
                    setattr(m, "has_multiple_results", len(getattr(m, "results")) > 1)

                    for pa in p.type.function_signature.parameters or []:
                        full_type_name = f"{pa.type.namespace}::{pa.type.name}"
                        setattr(
                            pa.type,
                            "is_result_base",
                            full_type_name in classes
                            and self.__class_derives_from(
                                classes[full_type_name], "csp::systems", "ResultBase", classes
                            ),
                        )

                    m.parameters.remove(p)

                    if len(m.parameters) > 0:
                        m.parameters[-1].is_last = True

        for c in classes.values():
            self.__translate_namespace(c)

            if c.doc_comments is not None:
                self.__translate_comments(c.doc_comments)

            if c.base is not None:
                self.__translate_namespace(c.base)
                setattr(c.base, "is_void_pointer", False)

            if c.interfaces is not None:
                for i in c.interfaces:
                    self.__translate_type(i.type)

            for f in c.fields:
                self.__translate_type(f.type)
                f.name = f.name[0].lower() + f.name[1:]

                if not f.type.is_template:
                    continue

                for ta in f.type.template_arguments or []:
                    self.__translate_type(ta.type)

            for m in c.methods:
                if m.doc_comments is not None:
                    self.__translate_comments(m.doc_comments)

                # TODO: Move this to Parser
                if m.return_type is None:
                    m.return_type = TypeMetadata(namespace=None, name="void")
                    setattr(m.return_type, "translated_namespace", None)
                    setattr(m.return_type, "is_void", True)
                else:
                    setattr(m.return_type, "is_void", False)

                assert m.name is not None

                # Make first character lowercase to match camelCase standard used in Typescript
                m.name = m.name[0].lower() + m.name[1:]

                self.__translate_type(m.return_type)

                if m.return_type.is_template:
                    for a in m.return_type.template_arguments or []:
                        self.__translate_type(a.type)

                setattr(m, "is_task", m.is_async_result or m.is_async_result_with_progress)

                for p in m.parameters or []:
                    self.__translate_type(p.type)

                    if p.type.is_template:
                        for ta in p.type.template_arguments or []:
                            self.__translate_type(ta.type)

                    if p.type.is_function_signature and p.type.function_signature is not None:
                        # TODO: Move this to Parser
                        if p.type.function_signature.return_type is None:
                            p.type.function_signature.return_type = TypeMetadata(namespace=None, name="void")
                            setattr(p.type.function_signature.return_type, "translated_namespace", None)
                            setattr(p.type.function_signature.return_type, "is_void", True)
                        else:
                            setattr(p.type.function_signature.return_type, "is_void", False)

                        self.__translate_type(p.type.function_signature.return_type)

                        for pa in p.type.function_signature.parameters or []:
                            self.__translate_type(pa.type)
                            pa.name = pa.name[0].lower() + pa.name[1:]

                    if p.name.startswith("In") and p.name[2].isupper():
                        p.name = p.name[2:]

                    p.name = p.name[0].lower() + p.name[1:]

                    if not m.is_async_result and not m.is_async_result_with_progress:
                        continue

                    if not p.type.is_function_signature:
                        continue

                    assert p.type.function_signature is not None
                    assert m.parameters is not None

                    setattr(m, "results", p.type.function_signature.parameters)
                    setattr(m, "has_multiple_results", len(getattr(m, "results")) > 1)

                    assert p.type.function_signature.parameters is not None

                    for pa in p.type.function_signature.parameters:
                        full_type_name = f"{pa.type.namespace}::{pa.type.name}"
                        setattr(
                            pa.type,
                            "is_result_base",
                            full_type_name in classes
                            and self.__class_derives_from(
                                classes[full_type_name], "csp::systems", "ResultBase", classes
                            ),
                        )

                    m.parameters.remove(p)

                    if len(m.parameters) > 0:
                        m.parameters[-1].is_last = True

        for t in templates.values():
            self.__translate_namespace(t.definition)

            for m in t.definition.methods:
                if m.doc_comments is not None:
                    self.__translate_comments(m.doc_comments)

                # TODO: Move this to Parser
                if m.return_type is None:
                    m.return_type = TypeMetadata(namespace=None, name="void")
                    setattr(m.return_type, "translated_namespace", None)
                    setattr(m.return_type, "is_void", True)
                else:
                    setattr(m.return_type, "is_void", False)

                assert m.name is not None

                m.name = m.name[0].lower() + m.name[1:]

                self.__translate_type(m.return_type)

                for p in m.parameters or []:
                    self.__translate_type(p.type)

                    p.name = p.name[0].lower() + p.name[1:]

            for i in t.instances:
                for a in i.arguments:
                    self.__translate_type(a.type)

        # Create output directory if it doesn't exist
        Path(self.__OUTPUT_DIRECTORY).mkdir(parents=True, exist_ok=True)

        template = read_whole_file(self.__TEMPLATE_DIRECTORY + "GeneratedWrapper.mustache")

        # Sort class metadata by namespace so that nested types always come after their parent type
        # This is needed as we combine the parent class with a namespace that contains the nested class,
        #  and this namespace needs to come after the class definition
        sorted_classes = sorted(classes.values(), key=lambda c: c.namespace)

        # Move all classes that have a base type to the end of the class list
        # This ensures that base classes are declared before derived classes
        resorted_classes: list[ClassMetadata] = []

        for c in sorted_classes:
            if c.base is None:
                resorted_classes.append(c)

        for c in sorted_classes:
            if c.base is not None:
                resorted_classes.append(c)

        rendered_functions = TypeScriptWrapperGenerator_Jinja2().generate(functions, classes, templates)

        with open(f"{self.__OUTPUT_DIRECTORY}connectedspacesplatform.ts", "w") as f:
            print(
                chevron.render(
                    template,
                    {
                        "enums": list(enums.values()),
                        "rendered_functions": rendered_functions["global_functions"],
                        "classes": resorted_classes,
                        "templates": list(templates.values()),
                        "interfaces": list(interfaces.values()),
                        "extra_data": config,
                    },
                    self.__PARTIALS_DIRECTORY,
                    warn=True,
                ),
                file=f,
            )

        # Run Prettier on output
        subprocess.run(f'npx prettier --write "{self.__OUTPUT_DIRECTORY}connectedspacesplatform.ts"', shell=True)

        # Compile output to Javascript
        if not Path(f"{self.__OUTPUT_DIRECTORY}/ConnectedSpacesPlatform_WASM.d.ts").is_file():
            shutil.copy2(f"{self.__OUTPUT_DIRECTORY}../../ConnectedSpacesPlatform_WASM.d.ts", self.__OUTPUT_DIRECTORY)

        subprocess.run(
            f'npx --package typescript tsc "{self.__OUTPUT_DIRECTORY}connectedspacesplatform.ts" -d -m es2020 -t es2020 --sourceMap',
            shell=True,
        )
