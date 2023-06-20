from copy import deepcopy
from pathlib import Path
from shutil import rmtree
from typing import Dict, List, Union

import subprocess
import chevron
import os
import sys

from Config import config
from MetadataTypes import EnumMetadata, StructMetadata, FunctionMetadata, ClassMetadata, TemplateMetadata, TypeMetadata, InterfaceMetadata
from Parser import read_whole_file, error_in_file, warning_in_file


class CSharpWrapperGenerator:
    __TEMPLATE_DIRECTORY = config['template_directory'] + 'CSharp/'
    __PARTIALS_DIRECTORY = __TEMPLATE_DIRECTORY + 'Partials/'
    __OUTPUT_DIRECTORY = config['output_directory'] + 'CSharp/'
    __BASE_NAMESPACE = 'Csp'

    __NAMESPACE_TRANSLATIONS = {
        'csp': __BASE_NAMESPACE,
        'common': 'Common',
        'memory': 'Memory',
        'multiplayer': 'Multiplayer',
        'services': 'Services',
        'systems': 'Systems',
        'web': 'Web'
    }


    def __translate_namespace(self, obj: any) -> None:
        if obj.namespace == None:
            setattr(obj, 'translated_namespace', None)

            return
        
        namespaces = obj.namespace.split('::')

        for i in range(0, min(2, len(namespaces))):
            if namespaces[i] in self.__NAMESPACE_TRANSLATIONS:
                namespaces[i] = self.__NAMESPACE_TRANSLATIONS[namespaces[i]]
        
        setattr(obj, 'translated_namespace', '.'.join(namespaces))


    def __translate_enum_base(self, obj: EnumMetadata) -> None:
        t = obj.base
        
        if t == 'uint8_t' or t == 'unsigned char':
            obj.base = 'byte'
        elif t =='uint16_t' or t == 'unsigned short':
            obj.base = 'ushort'


    def __translate_type(self, obj : TypeMetadata) -> None:
        self.__translate_namespace(obj)

        if obj.is_template:
            for ta in obj.template_arguments:
                self.__translate_type(ta.type)

        t = obj.name

        if not hasattr(obj, 'is_void_pointer'):
            setattr(obj, 'is_void_pointer', False)
        
        if t == 'int8_t':
            obj.name = 'sbyte'
        elif t == 'uint8_t' or t == 'unsigned char':
            obj.name = 'byte'
        elif t == 'int16_t':
            obj.name = 'short'
        elif t == 'uint16_t':
            obj.name = 'ushort'
        elif t == 'int32_t' or t == 'long':
            obj.name = 'int'
        elif t == 'uint32_t' or t == 'unsigned int' or t == 'unsigned long':
            obj.name = 'uint'
        elif t == 'int64_t' or t == 'long long' or t == 'long int':
            obj.name = 'long'
        elif t == 'uint64_t' or t == 'unsigned long long' or t == 'unsigned long int':
            obj.name = 'ulong'
        elif t == 'size_t':
            # Assume 64-bit only for now
            obj.name = 'ulong'
        elif t == 'String':
            obj.name = 'string'
            obj.namespace = None
            setattr(obj, 'translated_namespace', None)
            obj.is_pointer = False
            obj.is_reference = False
            obj.is_pointer_or_reference = False
        elif (t == 'void' or t == 'char') and obj.is_pointer:
            obj.name = 'IntPtr'
            obj.is_pointer = False
            obj.is_reference = False
            obj.is_pointer_or_reference = False
            setattr(obj, 'is_void_pointer', True)
            setattr(obj, 'translated_namespace', None)
    

    def __translate_comments(self, comments: List[str]) -> None:
        if comments == None:
            return
            
        for i in range(len(comments)):
            comment = comments[i]
            comment = comment.replace('<', '&lt;').replace('>', '&gt;')

            if comment[0] != '@':
                comments[i] = f"<remarks>{comment}</remarks>"
                continue

            index = comment.find(' ')
            tag = comment[:index]
            content = comment[index + 1:]

            if tag == '@brief':
                comments[i] = f"<summary>{content}</summary>"
            elif tag == '@return':
                index = content.find(':')

                while content[index + 1] == ':':
                    index = content.find(':', index + 2)

                #var_type = content[:index]
                content = content[index + 1:].lstrip()
                
                if content[0].islower():
                    content = content.capitalize()
                
                comments[i] = f"<returns>{content}</returns>"
            elif tag == '@param':
                index = content.find(' ')
                var_name = content[:index]
                content = content[index + 1:].lstrip()

                index = content.find(':')

                while content[index + 1] == ':':
                    index = content.find(':', index + 2)

                #var_type = content[:index]
                content = content[index + 1:].lstrip()
                
                if content[0].islower():
                    content = content.capitalize()
                
                comments[i] = f'<param name="{var_name}">{content}</param>'
    

    def __class_derives_from(self, obj: ClassMetadata, base_namespace: str, base_name: str, classes: Dict[str, ClassMetadata]) -> bool:
        if obj.base == None:
            return False
        
        if obj.base.namespace == base_namespace and obj.base.name == base_name:
            return True
        
        full_type_name = f"{obj.base.namespace}::{obj.base.name}"

        if not full_type_name in classes:
            return False

        base_class = classes[full_type_name]

        return self.__class_derives_from(base_class, base_namespace, base_name, classes)

    
    def __get_file_output_directory(self, obj: Union[EnumMetadata, StructMetadata, InterfaceMetadata, ClassMetadata, TemplateMetadata]):
        header_file: str

        if isinstance(obj, TemplateMetadata):
            header_file = obj.definition.header_file
        else:
            header_file = obj.header_file

        out_path = header_file.split('/')
        out_path = out_path[1:-1]

        return '/'.join(out_path)


    def generate(self, enums: Dict[str, EnumMetadata], structs: Dict[str, StructMetadata], functions: Dict[str, FunctionMetadata],
        classes: Dict[str, ClassMetadata], templates: Dict[str, TemplateMetadata], interfaces: Dict[str, InterfaceMetadata]
    ) -> None:
        # Deepcopy all metadata so we don't modify the original data for any wrapper generator classes that get called after this one
        enums = deepcopy(enums)
        structs = deepcopy(structs)
        functions = deepcopy(functions)
        classes = deepcopy(classes)
        templates = deepcopy(templates)
        interfaces = deepcopy(interfaces)

        out_path = Path(self.__OUTPUT_DIRECTORY)

        # Remove the output directory first to delete all previously generated code
        if out_path.exists():
            rmtree(out_path)

        # Recreate the output directory
        out_path.mkdir(parents=True, exist_ok=True)
        
        enum_template = read_whole_file(self.__TEMPLATE_DIRECTORY + 'Enum.mustache')
        struct_template = read_whole_file(self.__TEMPLATE_DIRECTORY + 'Struct.mustache')
        global_functions_template = read_whole_file(self.__TEMPLATE_DIRECTORY + 'GlobalFunctions.mustache')
        class_template = read_whole_file(self.__TEMPLATE_DIRECTORY + 'Class.mustache')
        interface_template = read_whole_file(self.__TEMPLATE_DIRECTORY + 'Interface.mustache')
        templateclass_template = read_whole_file(self.__TEMPLATE_DIRECTORY + 'Template.mustache')

        for e in enums.values():
            surrounding_types = None

            if e.is_nested_type:
                surrounding_types = e.namespace[e.namespace.find('::') + 2:].split('::')
                e.namespace = e.namespace[:e.namespace.find('::')]

            self.__translate_comments(e.doc_comments)
            self.__translate_namespace(e)
            self.__translate_enum_base(e)
            subdir = self.__get_file_output_directory(e)

            if surrounding_types != None:
                for st in surrounding_types:
                    subdir = f"{subdir}/{st}"

            setattr(e, 'surrounding_types', surrounding_types)

            for f in e.fields:
                self.__translate_comments(f.doc_comments)

            Path(self.__OUTPUT_DIRECTORY + subdir).mkdir(parents=True, exist_ok=True)
            
            with open(f"{self.__OUTPUT_DIRECTORY}{subdir}/{e.name}.cs", 'w') as f:
                print(chevron.render(enum_template, { 'data': e, 'extra_data': config }, self.__PARTIALS_DIRECTORY, warn=True), file=f)
        
        for s in structs.values():
            surrounding_types = None

            if s.is_nested_type:
                surrounding_types = s.namespace[s.namespace.find('::') + 2:].split('::')
                s.namespace = s.namespace[:s.namespace.find('::')]
            
            self.__translate_comments(s.doc_comments)
            self.__translate_namespace(s)
            subdir = self.__get_file_output_directory(s)

            if surrounding_types != None:
                for st in surrounding_types:
                    subdir = f"{subdir}/{st}"

            setattr(s, 'surrounding_types', surrounding_types)

            Path(self.__OUTPUT_DIRECTORY + subdir).mkdir(parents=True, exist_ok=True)

            with open(f"{self.__OUTPUT_DIRECTORY}{subdir}/{s.name}.cs", 'w') as f:
                print(chevron.render(struct_template, { 'data': s, 'extra_data': config }, self.__PARTIALS_DIRECTORY, warn=True), file=f)
        
        for f in functions.values():
            self.__translate_comments(f.doc_comments)

            if f.return_type != None:
                self.__translate_type(f.return_type)
            
            for p in f.parameters:
                self.__translate_type(p.type)

                if p.type.is_function_signature:
                    for fp in p.type.function_signature.parameters:
                        self.__translate_type(fp.type)
        
        with open(f"{self.__OUTPUT_DIRECTORY}Foundation.cs", 'w') as f:
            print(chevron.render(global_functions_template, { 'data': list(functions.values()), 'extra_data': config }, self.__PARTIALS_DIRECTORY, warn=True), file=f)

        for i in interfaces.values():
            surrounding_types = None

            if i.is_nested_type:
                surrounding_types = i.namespace[i.namespace.find('::') + 2:].split('::')
                i.namespace = i.namespace[:i.namespace.find('::')]

            self.__translate_comments(i.doc_comments)
            self.__translate_namespace(i)

            delegates = []
            events = []
            
            for m in i.methods:
                self.__translate_comments(m.doc_comments)

                if m.return_type != None:
                    self.__translate_type(m.return_type)

                    if m.return_type.is_template:
                        for ta in m.return_type.template_arguments:
                            self.__translate_type(ta.type)

                setattr(m, 'is_task', m.is_async_result or m.is_async_result_with_progress)

                if getattr(m, 'is_task', False) and m.doc_comments != None and len(m.doc_comments) > 0:
                    param = m.parameters[-1]
                    m.doc_comments = m.doc_comments[:-1]

                    if len(param.type.function_signature.doc_comments) > 0:
                        comment = param.type.function_signature.doc_comments[-1]
                        comment = comment.replace('<', '&lt;').replace('>', '&gt;')

                        comment_index = comment.find(' ')
                        tag = comment[:comment_index]

                        if tag != '@param':
                            error_in_file(m.filename, -1, 'Last doc comment must describe callback parameter')

                        content = comment[comment_index + 1:]
                        comment_index = content.find(' ')
                        # var_name = content[:comment_index]
                        content = content[comment_index + 1:].lstrip()

                        comment_index = content.find(':')

                        while content[comment_index + 1] == ':':
                            comment_index = content.find(':', comment_index + 2)

                        # var_type = content[:comment_index]
                        content = content[comment_index + 1:].lstrip()
                        
                        if content[0].islower():
                            content = content.capitalize()
                        
                        m.doc_comments.append(f"<returns>{content}</returns>")
                    else:
                        m.doc_comments.append("<returns>The result for the request</returns>")
                
                for p in m.parameters:
                    self.__translate_type(p.type)

                    if p.type.is_template:
                        for ta in p.type.template_arguments:
                            self.__translate_type(ta.type)
                    
                    if not m.is_async_result and not m.is_async_result_with_progress and not m.is_event:
                        continue

                    if not p.type.is_function_signature:
                        continue

                    setattr(m, 'results', p.type.function_signature.parameters)
                    setattr(m, 'has_results', len(p.type.function_signature.parameters) > 0)
                    setattr(m, 'has_multiple_results', len(getattr(m, 'results')) > 1)
                    
                    param_name = p.name[0].upper() + p.name[1:]
                    
                    for dp in p.type.function_signature.parameters:
                        self.__translate_type(dp.type)

                        full_type_name = f"{dp.type.namespace}::{dp.type.name}"
                        setattr(dp.type, 'is_result_base', full_type_name in classes and self.__class_derives_from(classes[full_type_name], 'oly_services', 'ResultBase', classes))
                    
                    delegate = {
                        'name': f"{m.name}{param_name}Delegate",
                        'method_name': m.name,
                        'return_type': p.type.function_signature.return_type,
                        'parameters': deepcopy(p.type.function_signature.parameters),
                        'has_parameters': len(p.type.function_signature.parameters) > 0,
                        'has_progress': m.is_async_result_with_progress
                    }
                    
                    delegates.append(delegate)
                    setattr(m, 'delegate', delegate)
                    setattr(p, 'delegate', delegate)

                    if m.is_event:
                        event_name = ""

                        if m.name.startswith('Set') and m.name.endswith('Callback'):
                            event_name = f"On{m.name[len('Set'):-len('Callback')]}"
                        else:
                            warning_in_file(m.header_file, m.start_line, "Event functions should follow the naming pattern 'SetXCallback'.")
                            event_name = m.name

                        event = {
                            'name': event_name,
                            'class_name': m.parent_class.name,
                            'method_name': m.name,
                            'unique_method_name': m.unique_name,
                            'parameters': deepcopy(p.type.function_signature.parameters),
                            'has_parameters': len(p.type.function_signature.parameters) > 0,
                            'has_multiple_parameters': len(p.type.function_signature.parameters) > 1,
                            'delegate': delegate
                        }

                        events.append(event)
                        setattr(m, 'event', event)
                    
                    m.parameters.remove(p)
                    
                    if len(m.parameters) > 0:
                        m.parameters[-1].is_last = True
            
            setattr(i, 'delegates', delegates)
            setattr(i, 'events', events)
            setattr(i, 'has_events', len(events) > 0)

            subdir = self.__get_file_output_directory(i)

            if surrounding_types != None:
                for st in surrounding_types:
                    subdir = f"{subdir}/{st}"

            setattr(i, 'surrounding_types', surrounding_types)

            Path(self.__OUTPUT_DIRECTORY + subdir).mkdir(parents=True, exist_ok=True)
            
            with open(f"{self.__OUTPUT_DIRECTORY}{subdir}/{i.name}.cs", 'w') as f:
                print(chevron.render(interface_template, { 'data': i, 'extra_data': config }, self.__PARTIALS_DIRECTORY, warn=True), file=f)
        
        for c in classes.values():
            surrounding_types = None

            if c.is_nested_type:
                surrounding_types = c.namespace[c.namespace.find('::') + 2:].split('::')
                c.namespace = c.namespace[:c.namespace.find('::')]

            self.__translate_comments(c.doc_comments)
            self.__translate_namespace(c)

            if c.base != None:
                self.__translate_namespace(c.base)

            delegates = []
            events = []
            
            for f in c.fields:
                self.__translate_comments(f.doc_comments)
                self.__translate_type(f.type)

                if f.type.is_template:
                    for ta in f.type.template_arguments:
                        self.__translate_type(ta.type)
            
            for m in c.methods:
                self.__translate_comments(m.doc_comments)

                if m.return_type != None:
                    self.__translate_type(m.return_type)

                    if m.return_type.is_template:
                        for ta in m.return_type.template_arguments:
                            self.__translate_type(ta.type)

                setattr(m, 'is_task', m.is_async_result or m.is_async_result_with_progress)

                if getattr(m, 'is_task', False) and m.doc_comments != None and len(m.doc_comments) > 0:
                    param = m.parameters[-1]
                    m.doc_comments = m.doc_comments[:-1]

                    if len(param.type.function_signature.doc_comments) > 0:
                        comment = param.type.function_signature.doc_comments[-1]
                        comment = comment.replace('<', '&lt;').replace('>', '&gt;')

                        comment_index = comment.find(' ')
                        tag = comment[:comment_index]

                        if tag != '@param':
                            error_in_file(m.filename, -1, 'Last doc comment must describe callback parameter')

                        content = comment[comment_index + 1:]
                        comment_index = content.find(' ')
                        # var_name = content[:comment_index]
                        content = content[comment_index + 1:].lstrip()

                        comment_index = content.find(':')

                        while content[comment_index + 1] == ':':
                            comment_index = content.find(':', comment_index + 2)

                        # var_type = content[:comment_index]
                        content = content[comment_index + 1:].lstrip()
                        
                        if content[0].islower():
                            content = content.capitalize()
                        
                        m.doc_comments.append(f"<returns>{content}</returns>")
                    else:
                        m.doc_comments.append("<returns>The result for the request</returns>")
                
                for p in m.parameters:
                    self.__translate_type(p.type)

                    if p.type.is_template:
                        for ta in p.type.template_arguments:
                            self.__translate_type(ta.type)

                    if not p.type.is_function_signature:
                        continue

                    setattr(m, 'results', p.type.function_signature.parameters)
                    setattr(m, 'has_results', len(p.type.function_signature.parameters) > 0)
                    setattr(m, 'has_multiple_results', len(getattr(m, 'results')) > 1)
                    
                    param_name = p.name[0].upper() + p.name[1:]
                    
                    for dp in p.type.function_signature.parameters:
                        self.__translate_type(dp.type)

                        full_type_name = f"{dp.type.namespace}::{dp.type.name}"
                        setattr(dp.type, 'is_result_base', full_type_name in classes and self.__class_derives_from(classes[full_type_name], 'oly_services', 'ResultBase', classes))
                    
                    delegate = {
                        'name': f"{m.name}{param_name}Delegate",
                        'method_name': m.name,
                        'return_type': p.type.function_signature.return_type,
                        'parameters': deepcopy(p.type.function_signature.parameters),
                        'has_parameters': len(p.type.function_signature.parameters) > 0,
                        'has_progress': m.is_async_result_with_progress,
                        'include_managed': not (m.is_async_result or m.is_async_result_with_progress or m.is_event)
                    }
                    
                    delegates.append(delegate)
                    setattr(m, 'delegate', delegate)

                    if not m.is_async_result and not m.is_async_result_with_progress and not m.is_event:
                        continue

                    if m.is_event:
                        event_name = ""

                        if m.name.startswith('Set') and m.name.endswith('Callback'):
                            event_name = f"On{m.name[len('Set'):-len('Callback')]}"
                        else:
                            warning_in_file(m.header_file, m.start_line, "Event functions should follow the naming pattern 'SetXCallback'.")
                            event_name = m.name

                        event = {
                            'name': event_name,
                            'class_name': m.parent_class.name,
                            'method_name': m.name,
                            'unique_method_name': m.unique_name,
                            'parameters': deepcopy(p.type.function_signature.parameters),
                            'has_parameters': len(p.type.function_signature.parameters) > 0,
                            'has_multiple_parameters': len(p.type.function_signature.parameters) > 1,
                            'delegate': delegate
                        }

                        events.append(event)
                        setattr(m, 'event', event)
                    
                    m.parameters.remove(p)
                    
                    if len(m.parameters) > 0:
                        m.parameters[-1].is_last = True
            
            setattr(c, 'delegates', delegates)
            setattr(c, 'events', events)
            setattr(c, 'has_events', len(events) > 0)

            subdir = self.__get_file_output_directory(c)

            if surrounding_types != None:
                for st in surrounding_types:
                    subdir = f"{subdir}/{st}"

            setattr(c, 'surrounding_types', surrounding_types)

            Path(self.__OUTPUT_DIRECTORY + subdir).mkdir(parents=True, exist_ok=True)
            
            with open(f"{self.__OUTPUT_DIRECTORY}{subdir}/{c.name}.cs", 'w') as f:
                print(chevron.render(class_template, { 'data': c, 'extra_data': config }, self.__PARTIALS_DIRECTORY, warn=True), file=f)
        
        for t in templates.values():
            self.__translate_namespace(t.definition)

            for m in t.definition.methods:
                if m.return_type != None:
                    self.__translate_type(m.return_type)
                
                for p in m.parameters:
                    self.__translate_type(p.type)

            subdir = self.__get_file_output_directory(t)
            Path(self.__OUTPUT_DIRECTORY + subdir).mkdir(parents=True, exist_ok=True)

            with open(f"{self.__OUTPUT_DIRECTORY}{subdir}/{t.definition.name}.cs", 'w') as f:
                print(chevron.render(templateclass_template, { 'data': t, 'extra_data': config }, self.__PARTIALS_DIRECTORY, warn=True), file=f)
        
        # Format output with CSharpier
        scriptDirectory = os.path.dirname(os.path.realpath(__file__))
        subprocess.run(f"\"{ scriptDirectory }\\Formatters\\CSharpier\\dotnet-csharpier.exe\" \"{self.__OUTPUT_DIRECTORY}\"", shell=True)