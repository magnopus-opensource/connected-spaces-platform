from __future__ import annotations

import os
import sys

from copy import deepcopy
from io import TextIOWrapper
from enum import Enum
from pathlib import Path
from typing import Dict, List, Union, Any

from Config import config
from MetadataTypes import *
from WordReader import WordReader


log_file: TextIOWrapper


COLOUR_ERROR = '\033[91m'    # Bright red
COLOUR_WARNING = '\033[93m'  # Bright yellow
COLOUR_RESET = '\033[0m'

PRIMITIVE_TYPES = [
    'int8_t',
    'uint8_t',
    'int16_t', 'short', 'signed short', 'short int', 'signed short int',
    'uint16_t', 'unsigned short', 'unsigned short int',
    'int32_t', 'int', 'signed int', 'long', 'signed long', 'long int', 'signed long int',
    'uint32_t', 'unsigned int', 'unsigned long', 'unsigned long int',
    'int64_t', 'long long', 'signed long long', 'long long int', 'signed long long int',
    'uint64_t', 'unsigned long long', 'unsigned long long int',
    'float',
    'double'
]


def error_in_file(filename: str, line: int, message: str) -> None:
    print(f"{COLOUR_ERROR}** Error in file: {filename} line: {line}  {message}{COLOUR_RESET}", file=sys.stderr)
    log_file.flush()
    log_file.close()
    sys.exit(1)


def warning_in_file(filename: str | None, line: int, message: str) -> None:
    print(f"{COLOUR_WARNING}** Warning in file: {filename} line: {line}  {message}{COLOUR_RESET}", file=sys.stderr)


def get_rel_path(filename: str) -> str:
    return os.path.relpath(filename, config['public_include_directory']).replace('\\', '/')


def read_whole_file(filename: str) -> str:
    file = open(filename, encoding='utf-8-sig')
    data = file.read()
    file.close()
    
    return data


def log(message: str, indent_level: int = 0, indent_width: int = 2) -> None:
    print(f"{'': <{indent_level * indent_width}}{message}", file=log_file)


class AccessModifier(Enum):
    PRIVATE = 1
    PROTECTED = 2
    PUBLIC = 3


class Parser:
    """
        Parses C++ header files and generates metadata that represents each type and function.

        NOTE: This is purpose-written for internal REWIND projects. As such, there are some assumptions made about the consumed code base. Do not expect it to work correctly on all C++ code.
    """


    enums: Dict[str, EnumMetadata] = {}
    structs: Dict[str, StructMetadata] = {}
    functions: Dict[str, FunctionMetadata] = {}
    classes: Dict[str, ClassMetadata] = {}
    typedefs: Dict[str, TypedefMetadata] = {}
    templates: Dict[str, TemplateMetadata] = {}
    interfaces: Dict[str, InterfaceMetadata] = {}

    indent = 0
    namespaces: List[str] = []


    __OPERATOR_OVERLOAD_TRANSLATIONS = {
        '+': 'op_Add',
        '-': 'op_Sub',
        '*': 'op_Mul',
        '/': 'op_Div',
        '%': 'op_Mod',
        '^': 'op_Xor',
        '&': 'op_And',
        '|': 'op_Or',
        '~': 'op_Comp',
        '!': 'op_Not',
        '=': 'op_Assign',
        '<': 'op_LT',
        '>': 'op_GT',
        '+=': 'op_AddAssign',
        '-=': 'op_SubAssign',
        '*=': 'op_MulAssign',
        '/=': 'op_DivAssign',
        '%=': 'op_ModAssign',
        '^=': 'op_XorAssign',
        '&=': 'op_AndAssign',
        '|=': 'op_OrAssign',
        '<<': 'op_LSH',
        '>>': 'op_RSH',
        '<<=': 'op_LSHAssign',
        '>>=': 'op_RSHAssign',
        '==': 'op_Equ',
        '!=': 'op_NEqu',
        '<=': 'op_LTE',
        '>=': 'op_GTE',
        '++': 'op_Inc',
        '--': 'op_Dec',
        '->': 'op_Member',
        '()': 'op_Call',
        '[]': 'op_Index'
    }


    def __exit_scope(self, reader: WordReader, current_scope: int, opening_char: str = '{', closing_char: str = '}'):
        while True:
            word = reader.next_word()
            if word == None:
                return

            if word == opening_char:
                current_scope += 1
            elif word == closing_char:
                current_scope -= 1

                if current_scope == 0:
                    break


    def __create_unique_function_name(self, function: FunctionMetadata) -> str:
        unique_name = None

        if function.parent_class != None:
            func_name = function.name
            class_name = function.parent_class.name

            if function.parent_class.is_template_instance:
                class_name += f"__{'_'.join([a.name for a in function.parent_class.template_arguments])}"

            assert func_name is not None

            if func_name.startswith('operator'):
                op = func_name[len('operator'):]
                func_name = self.__OPERATOR_OVERLOAD_TRANSLATIONS[op]
            
            if function.is_explicit_converter:
                func_name = 'Conv'
            elif function.is_constructor:
                func_name = 'Ctor'
            elif function.is_destructor:
                func_name = 'Dtor'

            if function.is_const:
                func_name += 'C'

            assert function.namespace is not None

            unique_name = f"{function.namespace.replace('::', '_')}_{class_name}_{func_name}"

        else:
            unique_name = f"{function.namespace.replace('::', '_')}_{function.name}"

        if not function.is_constructor and not function.is_destructor:
            if function.return_type != None:
                t = function.return_type

                unique_name += '_' + t.name.replace('<', '__').replace('>', '__')

                if t.is_pointer_pointer:
                    unique_name += 'PP'
                elif t.is_pointer:
                    unique_name += 'P'
                elif t.is_reference:
                    unique_name += 'R'
                
                if t.is_const:
                    unique_name += 'C'
            else:
                unique_name += '_void'

        if function.parameters is not None:
            for p in function.parameters:
                t = p.type
                unique_name += '_' + t.name.replace('<', '__').replace('>', '__')

                if t.is_pointer_pointer:
                    unique_name += 'PP'
                elif t.is_pointer:
                    unique_name += 'P'
                elif t.is_reference:
                    unique_name += 'R'
                
                if t.is_const:
                    unique_name += 'C'
        
        return unique_name


    def __read_doc_comments(self, reader: WordReader, doc_comments: List[str]) -> None:
        while True:
            next_char = reader.peek_char()

            while next_char == ' ' or next_char == '\n':
                reader.skip_char(1)
                next_char = reader.peek_char()

            next_char = reader.peek_char()

            if next_char != '/':
                break

            next_char = reader.peek_char(1)

            if next_char != '/':
                break

            next_char = reader.peek_char(2)

            if next_char != '/':
                break

            reader.skip_char(3)
            
            comment = reader.next_word(delimiters=['\n'], return_empty=True)
            
            if len(comment) > 0:
                doc_comments.append(comment)


    def __parse_type(self, reader: WordReader, word: str) -> tuple[TypeMetadata | None, str]:
        is_const = False
        is_pointer = False
        is_pointer_pointer = False
        is_reference = False
        function_signature: FunctionMetadata | None = None
        is_template = False
        template_arguments: List[TemplateArgumentMetadata] = []
        is_optional = False
        is_inline_forward = False

        # Handle inline forward declarations
        if word == 'class' or word == 'struct':
            word = reader.next_word()
            is_inline_forward = True

        if word == 'const':
            is_const = True
            word = reader.next_word()

        name = word

        namespace = ''
        word = reader.next_word()

        while word == ':':
            if namespace != '':
                namespace += '::'
            
            namespace += name
            reader.next_word()
            name = reader.next_word()
            word = reader.next_word()
        
        if name == 'Optional':
            is_optional = True
            word = reader.next_word()
            name = word

            namespace = ''
            word = reader.next_word()

            while word == ':':
                if namespace != '':
                    namespace += '::'
                
                namespace += name
                reader.next_word()
                name = reader.next_word()
                word = reader.next_word()
        
        if name == 'function':
            word = reader.next_word()
            func_return, word = self.__parse_type(reader, word)
            word = reader.next_word()
            parameters: List[ParameterMetadata] = []

            i = 1

            while True:
                if word == ')':
                    break

                if word == ',':
                    word = reader.next_word()

                param_type, word = self.__parse_type(reader, word)
                param_name: str

                if word == ',' or word == ')':
                    param_name = f"arg{i}"
                else:
                    param_name = word
                    word = reader.next_word()
                
                assert param_type is not None
                
                parameters.append(ParameterMetadata(
                    name=param_name,
                    type=param_type,
                    is_out=False,
                    is_in_out=False
                ))
                
                i += 1
            
            if len(parameters) > 0:
                parameters[-1].is_last = True
            
            reader.next_word()  # '>'
            word = reader.next_word()

            function_signature = FunctionMetadata(
                header_file=None,
                start_line=0,
                end_line=0,
                namespace=None,
                name=None,
                return_type=func_return,
                has_return=func_return != None,
                has_parameters=len(parameters) > 0,
                parameters=parameters
            )
        elif word == '<': # Found a templated instance!
            is_template = True
            word = reader.next_word()

            while word != '>':  # Please oh please don't use templated types as template arguments!
                if word == ',':
                    word = reader.next_word()

                argument_type, word = self.__parse_type(reader, word)

                assert argument_type is not None

                template_arguments.append(TemplateArgumentMetadata(
                    type=argument_type
                ))
            
            template_arguments[-1].is_last = True
            word = reader.next_word()
        
        if is_optional:
            word = reader.next_word()   # '>'

        if word == '*':
            is_pointer = True
            word = reader.next_word()

            if word == '*':
                is_pointer_pointer = True
                word = reader.next_word()
        elif word == '&':
            is_reference = True
            word = reader.next_word()
        
        if name == 'void' and not is_pointer and not is_reference:
            return (None, word)
        
        return (TypeMetadata(
            namespace=namespace,
            name=name,
            is_pointer=is_pointer,
            is_pointer_pointer=is_pointer_pointer,
            is_reference=is_reference,
            is_pointer_or_reference=is_pointer or is_reference,
            is_const=is_const, 
            is_string=name == 'String',
            is_function_signature=function_signature != None,
            function_signature=function_signature,
            is_template=is_template,
            template_name=name,
            template_arguments=template_arguments,
            is_optional=is_optional,
            is_inline_forward=is_inline_forward,
            is_primitive=name in PRIMITIVE_TYPES
        ), word)


    def __parse_enum(self, filename: str, reader: WordReader) -> EnumMetadata:
        word = reader.next_word()

        if word == 'class':
            word = reader.next_word()

        is_flags = False

        if word == config['enum_flags_macro']:
            is_flags = True
            word = reader.next_word()

        start_line = reader.current_line
        namespace = '::'.join(self.namespaces)

        name = word

        word = reader.next_word()
        base: str | None = None

        if word == ';':
             return

        log(f"Entering enum '{namespace}::{name}'", self.indent)
        self.indent += 1

        if word == ':':
            base = reader.next_word()
            word = reader.next_word()

            log(f"Base Type: {base}", self.indent)
        
        if word != '{':
            error_in_file(filename, reader.current_line, f"Expected '{{' or ';' after enum declaration. Got '{word}'.")

        fields: List[EnumFieldMetadata] = []
        
        word = reader.next_word()

        while word != '}':
            doc_comments: List[str] = []

            if word == '///':
                # Hack to go back to beginning of comment... Required for __read_doc_comments
                count = 0
                i = -1

                while True:
                    if reader.peek_char(i) == '/':
                        count += 1
                    else:
                        count = 0
                    
                    if count == 3:
                        break

                    i -= 1
                
                reader.skip_char(i)

                self.__read_doc_comments(reader, doc_comments)
                word = reader.next_word()

            field = word
            value: str | None = None
            word = reader.next_word()

            if word == ',':
                word = reader.next_word()
            elif word == '=':
                value = reader.next_word()
                word = reader.next_word()

                while (word != ',' and word != '}'):
                    value = value + word
                    word = reader.next_word()
                
                if word == ',':
                    word = reader.next_word()
            
            fields.append(EnumFieldMetadata(
                name=field,
                value=value
            ))

        log('Fields:', self.indent)
        self.indent += 1

        for f in fields:
            log(str(f), self.indent)

        self.indent -=1

        self.indent -= 1
        log(f"Exiting enum '{'::'.join(self.namespaces)}'", self.indent)

        reader.next_word()  # ';'

        return EnumMetadata(
            header_file=get_rel_path(filename),
            start_line=start_line,
            end_line=reader.current_line,
            namespace='::'.join(self.namespaces),
            name=name,
            full_safe_type_name='_'.join(namespace.split('::')) + "_" + name,
            base=base,
            fields=fields,
            is_flags=is_flags
        )


    def __parse_struct(self, filename: str, reader: WordReader) -> StructMetadata | None:
        start_line = reader.current_line

        word = reader.next_word()

        if word != config['cpp_export_macro']:
            word = reader.next_word()

            if word == ';':
                # Just a forward-declaration
                pass
            else:
                # Struct isn't exported, so skip it
                self.__exit_scope(reader, 1 if word == '{' else 0)
                reader.next_word()
            
            return None

        namespace = '::'.join(self.namespaces)
        name = reader.next_word()
        log(f"Entering struct '{namespace}::{name}'", self.indent)

        word = reader.next_word()
        base: str | None = None

        if word == ':':
            base = reader.next_word()
            word = reader.next_word()

            log(f"Base Type: {base}", self.indent)
        
        _struct = StructMetadata(
            header_file=get_rel_path(filename),
            start_line=start_line,
            namespace='::'.join(self.namespaces),
            name=name,
            base=base
        )

        self.indent += 1
        
        if word != '{':
            error_in_file(filename, reader.current_line, f"Expected '{{' after struct declaration. Got '{word}'.")
        
        word = reader.next_word()

        fields: List[FieldMetadata] = []

        while word != '}':
            if word == 'CSP_START_IGNORE':
                word = reader.next_word()

                while word != 'CSP_END_IGNORE':
                    word = reader.next_word()
            else:
                res = self.__parse_field(filename, reader, word, _struct)

                assert res is not None

                res.unique_getter_name = f"{res.namespace.replace('::', '_')}_{name}__Get_{res.name}"
                res.unique_setter_name = f"{res.namespace.replace('::', '_')}_{name}__Set_{res.name}"
                fields.append(res)

            word = reader.next_word()
        
        word = reader.next_word() # ';'

        log('Fields:', self.indent)
        self.indent += 1

        for f in fields:
            log(str(f), self.indent)

        self.indent -=1

        self.indent -= 1
        log(f"Exiting struct '{'::'.join(self.namespaces)}'", self.indent)

        _struct.end_line=reader.current_line
        _struct.fields=fields

        return _struct
    

    def __parse_field(self, filename: str, reader: WordReader, word: str, parent_type: Union[ClassMetadata, StructMetadata]) -> FieldMetadata | None:
        is_static = False

        field_type, word = self.__parse_type(reader, word)

        if word == ';': # forward declaration
            return None
        
        assert field_type is not None
        
        if field_type.is_inline_forward:
            #warning_in_file(get_rel_path(filename), reader.current_line, "Inline forward declarations are supported, but should not be used in Connected Spaces Platform code. Please consider moving forward declarations to the top of the file.")
            pass

        name = word
        word = reader.next_word()

        if word == '=':
            while word != ';':
                word = reader.next_word()
        
        return FieldMetadata(
            parent_type=parent_type,
            namespace='::'.join(self.namespaces),
            name=name,
            type=field_type,
            is_static=is_static
        )


    def __parse_function(self, filename: str, reader: WordReader, word: str) -> FunctionMetadata:
        is_async_result = False
        is_async_result_with_progress = False
        is_event = False
        is_const = False
        return_type = None
        is_constructor = False
        is_destructor = False
        is_virtual = False
        is_override = False

        start_line = reader.current_line

        if word == config['async_result_macro']:
            is_async_result = True
            word = reader.next_word()
        elif word == config['async_result_with_progress_macro']:
            is_async_result_with_progress = True
            word = reader.next_word()
        elif word == config['event_macro']:
            is_event = True
            word = reader.next_word()
        
        if word == 'virtual':
            is_virtual = True
            word = reader.next_word()

        if word == '~':
            word = word + reader.next_word()
        
        return_type, word = self.__parse_type(reader, word)

        if word == '(':
            assert return_type is not None

            # Constructor/Destructor
            word = return_type.name
            return_type = None

            if word[0] == '~':
                is_destructor = True
            else:
                is_constructor = True
        
        name = word

        _function = FunctionMetadata(
            header_file=get_rel_path(filename),
            start_line=start_line,
            end_line=0,
            namespace='::'.join(self.namespaces),
            name=name,
            return_type=return_type,
            has_return=return_type != None,
            has_parameters=False,
            parameters=None,
            is_virtual=is_virtual,
            is_override=False,
            is_constructor=is_constructor,
            is_destructor=is_destructor,
            is_const=is_const,
            is_async_result=is_async_result,
            is_async_result_with_progress=is_async_result_with_progress,
            is_event=is_event
        )

        if name.startswith('operator'):
            _function.is_operator_overload = True

            assert _function.name is not None

            if name == 'operator' and reader.peek_char() == '(':
                _function.name = 'operator()'
                reader.next_word()  # '('
                reader.next_word()  # ')'
            else:
                while reader.peek_char() != '(':
                    _function.name += reader.next_word()
            
            operator = _function.name[len('operator'):]

            # TODO: Add support for all operators
            if operator == '[]':
                _function.is_index_operator = True

                assert return_type is not None

                if not return_type.is_reference:
                    error_in_file(filename, reader.current_line, "Subscript/Index operator overload should return a reference so that the value is modifiable.")
            elif operator == '==':
                _function.is_equal_operator = True
            elif operator == '!=':
                _function.is_notequal_operator = True

        parameters: List[ParameterMetadata] = []
        
        if not is_constructor and not is_destructor:
            reader.next_word()  # '('
            
        word = reader.next_word()

        i = 1

        while True:
            if word == ')':
                break

            if word == ',':
                word = reader.next_word()

            is_out = False
            is_in_out = False

            if word == config['out_macro']:
                is_out = True
                word = reader.next_word()
            elif word == config['in_out_macro']:
                is_in_out = True
                word = reader.next_word()

            param_type, word = self.__parse_type(reader, word)
            param_name: str

            if word == ',' or word == ')':
                param_name = f"arg{i}"
            else:
                param_name = word
                word = reader.next_word()
            
            assert param_type is not None
            
            parameters.append(ParameterMetadata(
                name=param_name,
                type=param_type,
                is_out=is_out,
                is_in_out=is_in_out
            ))

            # Skip default values for now as parsing them won't be simple
            if word == '=':
                count = 0
                next_char = reader.peek_char()

                while next_char != ',' and next_char != ')':
                    count += 1
                    next_char = reader.peek_char(count)
                
                if count > 0:
                    reader.skip_char(count)
                
                word = reader.next_word()
            
            i += 1
        
        if len(parameters) > 0:
            parameters[-1].is_last = True
        
        word = reader.next_word()

        if word == 'const':
            is_const = True
            word = reader.next_word()
        
        if word == 'override':
            is_override = True
            word = reader.next_word()
        
        if word == '=':
            word = reader.next_word()   # Should be 'delete' or '0'

            if word == 'delete':
                _function.is_private = True

            word = reader.next_word()
        elif word == ':':
            word = reader.next_word()

            while word != '{':
                word = reader.next_word()

        if word == '{':
            self.__exit_scope(reader, 1)

            if reader.peek_char() == ';':
                word = reader.next_word()
        elif word != ';':
            error_in_file(filename, reader.current_line, 'WTF is going on here?')

        _function.has_parameters = len(parameters) > 0
        _function.parameters = parameters
        _function.is_const = is_const
        _function.is_override = is_override
        _function.end_line = reader.current_line
        
        return _function


    def __parse_class(self, filename: str, reader: WordReader, nested: bool = False) -> ClassMetadata | None:
        start_line = reader.current_line

        if not nested:
            word = reader.next_word()

            if word != config['cpp_export_macro']:
                name = word
                word = reader.next_word()

                if word == ';':
                    # Just a forward-declaration
                    pass
                else:
                    # Class isn't exported, so skip it
                    self.__exit_scope(reader, 1 if word == '{' else 0)
                    reader.next_word()
                
                return None
        
        should_dispose = True

        word = reader.next_word()

        if word == config['no_dispose_macro']:
            should_dispose = False
            word = reader.next_word()

        namespace = '::'.join(self.namespaces)
        name = word
        log(f"Entering class '{namespace}::{name}'", self.indent)
        self.indent += 1

        word = reader.next_word()
        inherits_from: List[TypeMetadata] = []

        if word == ':':
            while True:
                word = reader.next_word()

                if word == 'public' or word == 'private':
                    word = reader.next_word()

                inherited_type, word = self.__parse_type(reader, word)

                assert inherited_type is not None

                inherits_from.append(inherited_type)

                log(f"Inherited Type: {inherited_type}", self.indent)

                if word != ',':
                    break            
        
        if word != '{':
            error_in_file(filename, reader.current_line, f"Expected '{{' after class declaration. Got '{word}'.")

        fields: List[FieldMetadata] = []
        methods: List[FunctionMetadata] = []
        modifier = AccessModifier.PRIVATE
        is_pure_virtual = True
        class_is_static = True
        has_nested_types = False

        word = reader.next_word()

        _class = ClassMetadata(
            header_file=get_rel_path(filename),
            start_line=start_line,
            end_line=0,
            namespace=namespace,
            name=name,
            inherits_from=inherits_from,
            fields=fields,
            methods=methods,
            full_safe_type_name='_'.join(namespace.split('::')) + "_" + name,
            should_dispose=should_dispose
        )

        doc_comments: List[str] = []
        is_deprecated = False
        deprecation_message: str | None = None

        while word != '}':
            if word[0] == '/':
                if len(word) > 1:
                    if word[1] == '/':
                        if len(word) == 3 and word[2] == '/':   # Doxygen documentation
                            # Hack to go back to beginning of comment... Required for __read_doc_comments
                            count = 0
                            i = -1

                            while True:
                                if reader.peek_char(i) == '/':
                                    count += 1
                                else:
                                    count = 0
                                
                                if count == 3:
                                    break

                                i -= 1
                            
                            reader.skip_char(i)

                            self.__read_doc_comments(reader, doc_comments)
                        else:                                   # Single line comment
                            doc_comments.clear()
                            reader.skip_line()
                    else:
                        error_in_file(filename, reader.current_line, f"Expected '/' or '*' after '/'. Got '{word[1]}'.")
                elif reader.peek_char() == '*':
                    offset = 1

                    # Find the end of the block comment
                    while not (reader.peek_char(offset) == '*' and reader.peek_char(offset + 1) == '/'):
                        offset += 1
                    
                    offset += 2

                    reader.skip(offset)
            elif word == 'CSP_START_IGNORE':
                doc_comments.clear()
                is_deprecated = False
                deprecation_message = None
                word = reader.next_word()

                while word != 'CSP_END_IGNORE':
                    word = reader.next_word()
                
            elif word == '[':
                # doc_comments.clear()
                word = reader.next_word()

                if word != '[':
                    self.__exit_scope(reader, 1, opening_char='[', closing_char=']')
                else:
                    # Found an attribute!
                    word = reader.next_word()

                    if word == 'deprecated':
                        is_deprecated = True
                        word = reader.next_word()

                        if word == '(': # Deprecated attribute has a string description
                            if reader.peek_char() == '"':
                                reader.skip_char()
                                deprecation_message = reader.next_word(delimiters=['"'])
                                reader.skip_char()
                                
                            # Skip closing parens and brackets
                            reader.skip(3)  # ')]]'
                        elif word == ']':
                            # Skip closing bracket
                            reader.next_word()
                        else:
                            error_in_file(filename, reader.current_line, 'Malformed `deprecated` attribute found.')
                    else:
                        # Ignore attribute
                        self.__exit_scope(reader, 2, opening_char='[', closing_char=']')
            elif word == 'private':
                doc_comments.clear()
                is_deprecated = False
                deprecation_message = None
                modifier = AccessModifier.PRIVATE
                # Skip ':'
                reader.next_word()
            elif word == 'protected':
                doc_comments.clear()
                is_deprecated = False
                deprecation_message = None
                modifier = AccessModifier.PROTECTED
                reader.next_word()
            elif word == 'public':
                doc_comments.clear()
                is_deprecated = False
                deprecation_message = None
                modifier = AccessModifier.PUBLIC
                reader.next_word()
            elif word == 'friend':
                doc_comments.clear()
                is_deprecated = False
                deprecation_message = None
                word = reader.next_word()
                
                while word != ';':
                    word = reader.next_word()
            elif word == 'typedef':
                is_deprecated = False
                deprecation_message = None
                self.namespaces.append(name)
                word = reader.next_word()
                typedef_type, word = self.__parse_type(reader, word)
                typedef_name = word
                word = reader.next_word()

                if modifier == AccessModifier.PUBLIC:
                    assert typedef_type is not None

                    typedef = TypedefMetadata(
                        namespace='::'.join(self.namespaces),
                        name=typedef_name,
                        type=typedef_type,
                        doc_comments=deepcopy(doc_comments)
                    )
                    self.typedefs[f"{typedef.namespace}::{typedef.name}"] = typedef

                    log(f"Found typedef: {typedef}", self.indent)

                doc_comments.clear()
                self.namespaces.pop()
            elif word == 'using':
                doc_comments.clear()
                is_deprecated = False
                deprecation_message = None
                self.namespaces.append(name)
                word = reader.next_word()
                alias_name = word
                reader.next_word()  # '='
                word = reader.next_word()
                alias_type, word = self.__parse_type(reader, word)
                
                if modifier == AccessModifier.PUBLIC:
                    assert alias_type is not None

                    alias = TypedefMetadata(
                        namespace='::'.join(self.namespaces),
                        name=alias_name,
                        type=alias_type,
                        doc_comments=deepcopy(doc_comments)
                    )
                    self.typedefs[f"{alias.namespace}::{alias.name}"] = alias

                    log(f"Found using alias: {alias}", self.indent)
                    
                self.namespaces.pop()
            else:
                no_export = False
                is_explicit = False
                is_static = False

                if word == config['no_export_macro']:
                    doc_comments.clear()
                    is_deprecated = False
                    deprecation_message = None
                    no_export = True
                    word = reader.next_word()

                if word == 'static':
                    is_static = True
                    word = reader.next_word()
                
                class_is_static = class_is_static and is_static
                
                if word == 'explicit':
                    is_explicit = True
                    word = reader.next_word()
                
                hint = reader.find_next_of(['(', ';', '{'])

                if hint == '(':
                    # We found a function
                    if word == '~':
                        # Destructor
                        word = word + reader.next_word()

                    res = self.__parse_function(filename, reader, word)
                    res.doc_comments = deepcopy(doc_comments)
                    res.is_deprecated = is_deprecated
                    res.deprecation_message = deprecation_message

                    doc_comments.clear()
                    is_deprecated = False
                    deprecation_message = None

                    is_pure_virtual = is_pure_virtual and res.is_virtual

                    if not no_export and not any(m.name == res.name and m.parameters == res.parameters for m in methods):
                        res.class_name = name
                        res.is_static = is_static
                        res.is_explicit_converter = is_explicit
                        res.parent_class = _class
                        res.unique_name = self.__create_unique_function_name(res)
                        res.is_private = res.is_private or modifier != AccessModifier.PUBLIC

                        if modifier == AccessModifier.PUBLIC or (res.is_constructor or res.is_destructor):
                            methods.append(res)
                elif hint == '{':
                    is_deprecated = False
                    deprecation_message = None

                    # This must be a nested type
                    has_nested_types = True
                    self.namespaces.append(name)

                    if word == 'enum':
                        res = self.__parse_enum(filename, reader)

                        if res != None and modifier == AccessModifier.PUBLIC:
                            res.doc_comments = deepcopy(doc_comments)
                            res.is_nested_type = True
                            self.enums[f"{res.namespace}::{res.name}"] = res
                            
                        doc_comments.clear()
                    elif word == 'struct':
                        is_deprecated = False
                        deprecation_message = None
                        
                        res = self.__parse_struct(filename, reader)

                        if res != None and modifier == AccessModifier.PUBLIC:
                            res.doc_comments = deepcopy(doc_comments)
                            res.is_nested_type = True
                            self.structs[f"{res.namespace}::{res.name}"] = res
                            
                        doc_comments.clear()
                    elif word == 'class':
                        is_deprecated = False
                        deprecation_message = None
                        
                        res = self.__parse_class(filename, reader, nested=True)

                        if res != None and modifier == AccessModifier.PUBLIC:
                            res.doc_comments = deepcopy(doc_comments)
                            res.is_nested_type = True
                            self.classes[f"{res.namespace}::{res.name}"] = res
                        
                        doc_comments.clear()
                    
                    self.namespaces.pop()
                else:   # ';'
                    is_deprecated = False
                    deprecation_message = None

                    # Field!
                    res = self.__parse_field(filename, reader, word, _class)

                    if modifier == AccessModifier.PUBLIC and not no_export and res != None:
                        res.doc_comments = deepcopy(doc_comments)
                        res.is_static = is_static
                        res.unique_getter_name = f"{res.namespace.replace('::', '_')}_{name}__Get_{res.name}"
                        res.unique_setter_name = f"{res.namespace.replace('::', '_')}_{name}__Set_{res.name}"
                        fields.append(res)
                    
                    is_pure_virtual = False
                    doc_comments.clear()
            
            word = reader.next_word()
        
        # Add a default constructor if none exists
        # TODO: Handle deleted default constructors
        if not is_pure_virtual and not class_is_static:
            if not any(m.is_constructor for m in methods):
                ctor = FunctionMetadata(
                    header_file=get_rel_path(filename),
                    start_line=0,
                    end_line=0,
                    parent_class=_class,
                    namespace=namespace,
                    name=name,
                    return_type=None,
                    has_return=False,
                    has_parameters=False,
                    parameters=[],
                    is_constructor=True
                )
                ctor.unique_name = self.__create_unique_function_name(ctor)
                methods.append(ctor)

            if not any(m.is_destructor for m in methods):
                dtor = FunctionMetadata(
                    header_file=get_rel_path(filename),
                    start_line=0,
                    end_line=0,
                    parent_class=_class,
                    namespace=namespace,
                    name=name,
                    return_type=None,
                    has_return=False,
                    has_parameters=False,
                    parameters=[],
                    is_destructor=True
                )
                dtor.unique_name = self.__create_unique_function_name(dtor)
                methods.append(dtor)
        
        _class.end_line=reader.current_line
        _class.is_pure_virtual=is_pure_virtual
        _class.is_static=class_is_static
        _class.has_nested_types=has_nested_types
        
        if class_is_static:
            _class.should_dispose = False
        
        if any(m.is_destructor and m.is_private for m in methods):
            _class.should_dispose = False

        log('Fields:', self.indent)
        self.indent += 1

        for f in fields:
            log(str(f), self.indent)

        self.indent -=1

        log('Methods:', self.indent)
        self.indent += 1

        for m in methods:
            log(str(m), self.indent)

        self.indent -=1

        self.indent -= 1
        log(f"Exiting class '{namespace}::{name}'", self.indent)

        word = reader.next_word()  # ';'

        return _class
    

    def __parse_interface(self, filename: str, reader: WordReader, nested: bool = False) -> InterfaceMetadata | None:
        word = reader.next_word()

        if word != 'class':
            error_in_file(filename, reader.current_line, f"Expected 'class' after interface macro. Got '{word}'.")

        _class = self.__parse_class(filename, reader, nested)

        if _class == None:
            return None

        if _class.name[0] != 'I' and not _class.name[1].isupper():
            warning_in_file(filename, _class.start_line, "Interfaces should be named using the pattern 'IMyInterface'.")
        
        if len(_class.fields) > 0:
            error_in_file(filename, _class.start_line, 'Interfaces must not contain fields.')
        
        if not _class.is_pure_virtual:
            error_in_file(filename, _class.start_line, 'Interfaces must be pure virtual classes.')
        
        if _class.has_base_type:
            error_in_file(filename, _class.start_line, 'Interfaces must not inherit from any types except other interfaces.')
        
        destructor_found = False
        
        for m in _class.methods:
            if m.is_constructor:
                error_in_file(filename, _class.start_line, 'Interfaces must not define a constructor.')
            
            if m.is_destructor:
                destructor_found = True

                if not m.is_virtual:
                    error_in_file(filename, _class.start_line, 'Interface destructor must be virtual.')
        
        if not destructor_found:
            error_in_file(filename, _class.start_line, 'Interfaces must define a virtual destructor.')
        
        interfaces: List[ClassInterfaceMetadata] | None = None
        has_interfaces: bool = False
        
        if _class.inherits_from is not None and len(_class.inherits_from) > 0:
            interfaces = []
            has_interfaces = True

            for i in _class.inherits_from:
                interfaces.append(
                    ClassInterfaceMetadata(
                        name = i.name,
                        type = i
                    )
                )
            
            interfaces[-1].is_last = True

        return InterfaceMetadata(
            header_file=_class.header_file,
            start_line=_class.start_line,
            end_line=_class.end_line,
            namespace=_class.namespace,
            name=_class.name,
            methods=_class.methods,
            is_nested_type=_class.is_nested_type,
            full_safe_type_name=_class.full_safe_type_name,
            interfaces=interfaces,
            has_interfaces=has_interfaces
        )
    

    def __find_type_namespace(self, name: str, current_namespace: str, types: Dict[str, Any]) -> str | None:
        while True:
            full_type_name = f"{current_namespace}::{name}"

            if full_type_name in types:
                _type = types[full_type_name]

                if isinstance(_type, TemplateMetadata):
                    return _type.definition.namespace
                else:
                    return types[full_type_name].namespace
            
            pos = current_namespace.rfind('::')

            if pos == -1:
                return None
            
            current_namespace = current_namespace[:pos]
    

    def __find_and_set_type_namespace(self, object_: Any, object_type: TypeMetadata, parent_namespace: str, types: Dict[str, Any]) -> None:
        if object_type.namespace == '':
            object_type.namespace = self.__find_type_namespace(object_type.name, parent_namespace, types)
        
        if object_type.namespace == None:
            object_type.namespace = self.__find_type_namespace(object_type.name, parent_namespace, self.typedefs)
        
        if object_type.namespace == None:
            object_type.namespace = self.__find_type_namespace(object_type.name, parent_namespace, self.templates)
        
        if object_type.is_template:
            assert object_type.template_arguments is not None

            for a in object_type.template_arguments:
                if a.type.namespace == '':
                    a.type.namespace = self.__find_type_namespace(a.type.name, parent_namespace, types)
        
        full_type_name: str

        if object_type.namespace != None:
            full_type_name = f"{object_type.namespace}::{object_type.name}"
        else:
            full_type_name = object_type.name

        if full_type_name in self.typedefs:
            td = self.typedefs[full_type_name]
            object_type.name = td.type.name
            object_type.namespace = td.type.namespace
            object_type.is_string = td.type.is_string

            if td.type.is_function_signature:
                assert td.type.function_signature is not None

                object_type.is_function_signature = True
                object_type.function_signature = deepcopy(td.type.function_signature)

                if td.doc_comments != None:
                    object_type.function_signature.doc_comments = td.doc_comments
            
        if object_type.is_function_signature:
            assert object_type.function_signature is not None

            object_type.function_signature.name = object_.name

            if object_type.function_signature.parameters is not None:
                for p in object_type.function_signature.parameters:
                    self.__find_and_set_type_namespace(p, p.type, parent_namespace, types)
        
        if full_type_name not in types:
            return
            
        object_type_type = types[full_type_name]

        if object_type_type is ClassMetadata and object_type_type.base != None:
            self.__find_and_set_type_namespace(object_type_type, object_type_type.base, object_type_type.namespace, types)


    def __find_and_set_templated_type(self, object_type: TypeMetadata, parent_namespace: str, types: Dict[str, Any]) -> None:
        template_name = f"{object_type.namespace}::{object_type.name}"
        template = self.templates[template_name]

        if template.instances == None:
            template.instances = []
        
        assert object_type.template_arguments is not None

        template_arguments = deepcopy(object_type.template_arguments)

        for a in template_arguments:
            self.__find_and_set_type_namespace(a, a.type, parent_namespace, types)

        match = False
        
        for t in template.instances:
            match = True
            index = 0

            for a in t.arguments:
                match = match and (template_arguments[index].type.namespace == a.type.namespace)
                match = match and (template_arguments[index].type.name == a.type.name)
                index = index + 1
            
            if match:
                break
        
        if not match:
            template.instances.append(
                TemplateInstanceMetadata(
                    parent_namespace=parent_namespace,
                    template_parameters=template.template_parameters,
                    arguments=template_arguments
                )
            )
    

    def __set_type_classification(self, object_type: TypeMetadata) -> None:
        if object_type.namespace != None:
            full_type_name = f"{object_type.namespace}::{object_type.name}"

            if full_type_name in self.classes:
                object_type.is_class = True
                object_type.is_class_or_interface = True
            elif full_type_name in self.interfaces:
                object_type.is_interface = True
                object_type.is_class_or_interface = True
            elif full_type_name in self.templates:
                object_type.is_class = True
                object_type.is_class_or_interface = True
            elif full_type_name in self.enums:
                object_type.is_enum = True
    

    def __set_safe_type_name(self, object) -> None:        
        # Get safe type name for appending to end of method names
        name = object.type.name

        if name == 'bool':
            name = 'Bool'
        elif name == 'uint16_t' or name == 'unsigned short' or name == 'unsigned short int':
            name = 'UInt16'
        elif name == 'int16_t' or name == 'signed short' or name == 'signed short int' or name == 'short' or name == 'short int':
            name = 'Int16'
        elif name == 'size_t' or name == 'uint32_t' or name == 'unsigned int' or name == 'unsigned long' or name == 'unsigned long int':
            name = 'UInt32'
        elif name == 'int32_t' or name == 'signed int' or name == 'signed long' or name == 'signed long int' or name == 'int' or name == 'long' or name == 'long int':
            name = 'Int32'
        elif name == 'float':
            name = 'Float'
        elif name == 'double':
            name = 'Double'

        object.safe_type_name = name

        if object.type.namespace != None and name != 'String':
            namespaces = object.type.namespace.split('::')
            object.full_safe_type_name = '_'.join(namespaces) + '_' + name
        else:
            object.full_safe_type_name = name
        
        if object.type.is_template:
            for ta in object.type.template_arguments:
                self.__set_safe_type_name(ta)
                object.full_safe_type_name += '_' + ta.full_safe_type_name


    def parse(self, headers: List[str]) -> None:
        global log_file

        # Create output directory if it doesn't exist
        Path(config['output_directory']).mkdir(parents=True, exist_ok=True)
        log_file = open(f"{config['output_directory']}log.parser.txt", 'w')

        # Stage 1 processing
        for h in headers:
            log(f"Begin parsing {get_rel_path(h)}")
            self.indent += 1

            reader = WordReader(read_whole_file(h))
            reader.skip_whitespace()

            doc_comments: List[str] = []

            while True:
                next_char = reader.peek_char()

                # Break if we hit the end of the file
                if next_char == None:
                    break

                # Read comments
                if next_char == '/':
                    next_char = reader.peek_char(1)

                    if next_char == '/':
                        next_char = reader.peek_char(2)

                        if next_char == '/':    # Doxygen documentation
                            self.__read_doc_comments(reader, doc_comments)
                        else:                   # Single line comment
                            reader.skip_line()

                        continue
                    elif next_char == '*':
                        doc_comments.clear()
                        offset = 2

                        # Find the end of the block comment
                        while not (reader.peek_char(offset) == '*' and reader.peek_char(offset + 1) == '/'):
                            offset += 1
                        
                        offset += 2

                        reader.skip(offset)

                        continue

                word = reader.next_word()

                if word == None:
                    break

                # TODO: Look at this... It kinda seems like this will stop processing the file if it detects a top-level CSP_NO_EXPORT. Obviously, this is not what we want!
                if word == config['no_export_macro']:
                    break

                if word == '#':
                    doc_comments.clear()
                    current_line = reader.current_line

                    if reader.next_word() == 'pragma':
                        if reader.next_word() == 'pack':
                            error_in_file(h, current_line, 'Pragma pack is currently not supported. Please do not use it as it will break struct marshalling.')
                    else:
                        # Skip all preprocessor directives
                        reader.skip_line()
                elif word == '[':
                    doc_comments.clear()
                    self.__exit_scope(reader, 1, opening_char='[', closing_char=']')
                elif word == 'CSP_START_IGNORE':
                    word = reader.next_word()

                    while word != 'CSP_END_IGNORE':
                        word = reader.next_word()
                elif word == '}':
                    # We've exited a namespace
                    doc_comments.clear()
                    self.indent -= 1
                    log(f"Exiting namespace '{'::'.join(self.namespaces)}'", self.indent)
                    self.namespaces.pop()
                    
                    if reader.peek_char() == ';':
                        reader.next_word()
                elif word == 'namespace':
                    doc_comments.clear()
                    assert not self.namespaces
                    namespace = reader.next_word()
                    word = reader.next_word()

                    while word == ':':
                        reader.next_word()  # ':'
                        namespace = namespace + '::' + reader.next_word()
                        word = reader.next_word()

                    self.namespaces.append(namespace)

                    if word != '{':
                        error_in_file(h, reader.current_line, f"Expected '{{' after namespace declaration. Got '{word}'.")
                    
                    log(f"Entering namespace '{'::'.join(self.namespaces)}'", self.indent)
                    self.indent += 1
                elif word == 'enum':
                    res = self.__parse_enum(h, reader)

                    if res != None:
                        res.doc_comments = deepcopy(doc_comments)
                        self.enums[f"{res.namespace}::{res.name}"] = res
                    
                    doc_comments.clear()
                elif word == 'struct':
                    res = self.__parse_struct(h, reader)

                    if res != None:
                        res.doc_comments = deepcopy(doc_comments)
                        self.structs[f"{res.namespace}::{res.name}"] = res
                    
                    doc_comments.clear()
                elif word == 'class':
                    res = self.__parse_class(h, reader)

                    if res != None:
                        res.doc_comments = deepcopy(doc_comments)
                        self.classes[f"{res.namespace}::{res.name}"] = res
                    
                    doc_comments.clear()
                elif word == 'typedef':
                    word = reader.next_word()
                    typedef_type, word = self.__parse_type(reader, word)
                    typedef_name = word
                    word = reader.next_word()

                    assert typedef_type is not None

                    typedef = TypedefMetadata(
                        namespace='::'.join(self.namespaces),
                        name=typedef_name,
                        type=typedef_type,
                        doc_comments=deepcopy(doc_comments)
                    )
                    doc_comments.clear()
                    self.typedefs[f"{typedef.namespace}::{typedef.name}"] = typedef
                    
                    log(f"Found typedef: {typedef}", self.indent)
                elif word == 'using':
                    word = reader.next_word()
                    alias_name = word
                    reader.next_word()  # '='
                    word = reader.next_word()
                    alias_type, word = self.__parse_type(reader, word)

                    assert alias_type is not None

                    alias = TypedefMetadata(
                        namespace='::'.join(self.namespaces),
                        name=alias_name,
                        type=alias_type,
                        doc_comments=deepcopy(doc_comments)
                    )
                    doc_comments.clear()
                    self.typedefs[f"{alias.namespace}::{alias.name}"] = alias

                    log(f"Found using alias: {alias}", self.indent)
                elif word == 'template':
                    doc_comments.clear()
                    reader.next_word()  # '<'
                    word = reader.next_word()

                    parameters: List[TemplateParameterMetadata] = []

                    while word != '>':
                        if word == ',':
                            word = reader.next_word()

                        word = reader.next_word()   # 'typename', 'class', 'int', etc
                        parameters.append(TemplateParameterMetadata(word))

                        word = reader.next_word()
                    
                    parameters[-1].is_last = True

                    word = reader.next_word()

                    if word == 'class':
                        res = self.__parse_class(h, reader)

                        assert res is not None

                        self.templates[f"{res.namespace}::{res.name}"] = TemplateMetadata(
                            definition=res,
                            template_parameters=parameters
                        )
                    else:
                        error_in_file(h, reader.current_line, "Only class templates are currently supported.")
                elif word == config['interface_macro']:
                    res = self.__parse_interface(h, reader)

                    if res != None:
                        res.doc_comments = deepcopy(doc_comments)
                        self.interfaces[f"{res.namespace}::{res.name}"] = res
                    
                    doc_comments.clear()
                else:
                    # Assume we found a function

                    if word == config['cpp_export_macro']:
                        word = reader.next_word()
                        res = self.__parse_function(h, reader, word)
                        res.unique_name = self.__create_unique_function_name(res)
                        res.doc_comments = deepcopy(doc_comments)
                        self.functions[f"{res.namespace}::{res.name}"] = res
                        log(f"Found function: {res}", self.indent)
                        doc_comments.clear()
                    else:
                        doc_comments.clear()
                        log(f"Skipping unexported function", self.indent)
                        self.__parse_function(h, reader, word)

            self.indent -= 1
            log(f"End parsing {get_rel_path(h)}\n")

        # Stage 2 processing (templates)
        types = { **self.classes, **self.enums, **self.structs, **self.interfaces }

        # Add template instances for typedef parameters
        for t in self.typedefs.values():
            if not t.type.is_function_signature:
                continue

            assert t.type.function_signature is not None

            if t.type.function_signature.parameters is not None:
                for p in t.type.function_signature.parameters:
                    if p.type.is_template:
                        self.__find_and_set_templated_type(p.type, t.namespace, types)

                        assert p.type.template_arguments is not None 

                        for ta in p.type.template_arguments:
                            self.__set_type_classification(ta.type)

        # Add template instances for parameters in class functions and class fields
        for c in self.classes.values():
            for m in c.methods:
                if m.return_type != None and m.return_type.is_template:
                    self.__find_and_set_templated_type(m.return_type, c.namespace, types)
                
                if m.parameters is not None:
                    for p in m.parameters:
                        if p.type.is_template:
                            self.__find_and_set_templated_type(p.type, c.namespace, types)
            
            for f in c.fields:
                if f.type.is_template:
                    self.__find_and_set_templated_type(f.type, c.namespace, types)
        
        for t in self.templates.values():
            for f in t.definition.fields:
                if any(f.type.name == tp.name for tp in t.template_parameters):
                    f.type.is_template_argument = True

            for m in t.definition.methods:
                if m.return_type != None:
                    if any(m.return_type.name == tp.name for tp in t.template_parameters):
                        m.return_type.is_template_argument = True
                    
                    if m.return_type.is_template:
                        assert m.return_type.template_arguments is not None

                        for ta in m.return_type.template_arguments:
                            if any(ta.type.name == tp.name for tp in t.template_parameters):
                                ta.type.is_template_argument = True
                
                if m.parameters is not None:
                    for p in m.parameters:
                        if any(p.type.name == tp.name for tp in t.template_parameters):
                            p.type.is_template_argument = True

                        if p.type.is_template:
                            assert p.type.template_arguments is not None

                            for ta in p.type.template_arguments:
                                if any(ta.type.name == tp.name for tp in t.template_parameters):
                                    ta.type.is_template_argument = True

        # Stage 3 processing (type namespaces)
        for t in self.typedefs.values():
            self.__find_and_set_type_namespace(t, t.type, t.namespace, types)

        for f in self.functions.values():
            if not f.return_type is None:
                self.__find_and_set_type_namespace(f, f.return_type, f.namespace, types)
                self.__set_type_classification(f.return_type)
            
            if f.parameters is not None:
                for p in f.parameters:
                    self.__find_and_set_type_namespace(p, p.type, f.namespace, types)
                    self.__set_type_classification(p.type)

        for i in self.interfaces.values():
            for m in i.methods:
                if m.return_type != None:
                    self.__find_and_set_type_namespace(m, m.return_type, f"{i.namespace}::{i.name}", types)
                    self.__set_type_classification(m.return_type)

                    if m.return_type.is_template and not m.return_type.template_arguments is None:
                        for a in m.return_type.template_arguments:
                            self.__set_safe_type_name(a)
                
                if m.parameters is not None:
                    for p in m.parameters:
                        self.__find_and_set_type_namespace(p, p.type, f"{i.namespace}::{i.name}", types)
                        self.__set_type_classification(p.type)

                        if p.type.is_function_signature and not p.type.function_signature is None:
                            if p.type.function_signature.parameters is not None:
                                for fp in p.type.function_signature.parameters:
                                    self.__find_and_set_type_namespace(fp, fp.type, f"{i.namespace}::{i.name}", types)
                                    self.__set_type_classification(fp.type)

                                    if fp.type.is_template and not fp.type.template_arguments is None:
                                        for fpp in fp.type.template_arguments:
                                            self.__set_safe_type_name(fpp)
            
            if i.interfaces is not None:
                for ii in i.interfaces:
                    self.__find_and_set_type_namespace(ii, ii.type, f"{i.namespace}::{i.name}", types)
                    self.__set_type_classification(ii.type)
        
        for c in self.classes.values():
            for i in c.inherits_from:
                self.__find_and_set_type_namespace(c, i, c.namespace, types)
                self.__set_type_classification(i)

                if i.is_class:
                    if not c.base is None:
                        error_in_file(c.header_file, c.start_line, 'Classes must only inherit from a single base class. All other inherited types must be interfaces.')
                    
                    c.base = i
                    c.has_base_type = True
                elif i.is_interface:
                    if c.interfaces is None:
                        c.interfaces = []
                        c.has_interfaces = True

                    c.interfaces.append(
                        ClassInterfaceMetadata(
                            name = i.name,
                            type = i
                        )
                    )

            if not c.interfaces is None:
                c.interfaces[-1].is_last = True

            c.inherits_from = []
            
            for m in c.methods:
                if not c.interfaces is None:
                    for i in c.interfaces:
                        interface = self.interfaces[f"{i.type.namespace}::{i.type.name}"]
                        found_implementation = False

                        for im in interface.methods:
                            if im.name == m.name:
                                m.is_interface_implementation = True
                                found_implementation = True
                                break
                        
                        # Also look for function declaration in interface interfaces
                        # TODO: Break this logic off into its own function to allow recursion
                        if interface.interfaces is not None:
                            for ii in interface.interfaces:
                                _interface = self.interfaces[f"{ii.type.namespace}::{ii.type.name}"]

                                for im in _interface.methods:
                                    if im.name == m.name:
                                        m.is_interface_implementation = True
                                        found_implementation = True
                                        break
                        
                        if found_implementation:
                            break

                if not m.return_type is None:
                    self.__find_and_set_type_namespace(m, m.return_type, f"{c.namespace}::{c.name}", types)
                    self.__set_type_classification(m.return_type)

                    if m.return_type.is_template and not m.return_type.template_arguments is None:
                        for a in m.return_type.template_arguments:
                            self.__find_and_set_type_namespace(a, a.type, f"{c.namespace}::{c.name}", types)
                            self.__set_type_classification(a.type)
                            self.__set_safe_type_name(a)
                
                if m.parameters is not None:
                    for p in m.parameters:
                        self.__find_and_set_type_namespace(p, p.type, f"{c.namespace}::{c.name}", types)
                        self.__set_type_classification(p.type)

                        if p.type.is_function_signature and not p.type.function_signature is None:
                            if p.type.function_signature.parameters is not None:
                                for fp in p.type.function_signature.parameters:
                                    self.__find_and_set_type_namespace(fp, fp.type, f"{c.namespace}::{c.name}", types)
                                    self.__set_type_classification(fp.type)

                                    if fp.type.is_template and not fp.type.template_arguments is None:
                                        for fpp in fp.type.template_arguments:
                                            self.__set_safe_type_name(fpp)

            for f in c.fields:
                self.__find_and_set_type_namespace(f, f.type, f"{c.namespace}::{c.name}", types)
                self.__set_type_classification(f.type)

                if f.type.is_template and not f.type.template_arguments is None:
                    for fta in f.type.template_arguments:
                        self.__find_and_set_type_namespace(fta, fta.type, f"{c.namespace}::{c.name}", types)
                        self.__set_safe_type_name(fta)
                        self.__set_type_classification(fta.type)
        
        for t in self.templates.values():
            if t.instances == None:
                t.instances = []
            
            for i in t.instances:
                idx = 0

                for a in i.arguments:
                    if a.type.namespace == None:
                        tm = TypeMetadata(namespace=a.type.namespace, name=a.type.name, is_template=False)
                        self.__find_and_set_type_namespace(a, tm, i.parent_namespace, types)
                        a.type.namespace = tm.namespace
                    
                    a.parameter_name = t.template_parameters[idx].name
                    self.__set_safe_type_name(a)
                    self.__set_type_classification(a.type)

                    idx += 1
            
            for m in t.definition.methods:
                if m.return_type != None:
                    if not m.return_type.is_template_argument:
                        self.__find_and_set_type_namespace(m, m.return_type, f"{t.definition.namespace}::{t.definition.name}", types)
                        self.__set_type_classification(m.return_type)
                    
                    if m.return_type.is_template and not m.return_type.template_arguments is None:
                        idx = 0
                        template_name = f"{m.return_type.namespace}::{m.return_type.name}"
                        template = self.templates[template_name]

                        for a in m.return_type.template_arguments:
                            a.parameter_name = template.template_parameters[idx].name

                            idx += 1
                        
                        m.return_type.template_safe_type_name = template.definition.full_safe_type_name
                
                if m.parameters is not None:
                    for p in m.parameters:
                        if p.type.is_template_argument:
                            continue

                        self.__find_and_set_type_namespace(p, p.type, f"{t.definition.namespace}::{t.definition.name}", types)
                        self.__set_type_classification(p.type)
        
        # Stage 4 processing (further template processing)
        # Add template instances for return types that are templates
        for t in self.templates.values():
            if not t.instances is None:
                instances = t.instances.copy()  # Create a shallow copy as t.instances might be modified
            else:
                instances = []
            
            for i in instances:
                for m in t.definition.methods:
                    if m.return_type != None and m.return_type.is_template and not m.return_type.template_arguments is None:
                        return_type = deepcopy(m.return_type)   # Create a copy, as we don't want to actually modify the return type, but instead only add an instance for it
                        index = 0

                        assert return_type.template_arguments is not None

                        for a in return_type.template_arguments:
                            tp_type = next((x for x in i.arguments if x.parameter_name == a.type.name), None)

                            if tp_type == None:
                                index = index + 1
                                continue

                            is_last = a.is_last
                            parameter_name = a.parameter_name

                            return_type.template_arguments[index] = deepcopy(tp_type)
                            return_type.template_arguments[index].is_last = is_last
                            return_type.template_arguments[index].parameter_name = parameter_name
                            index = index + 1
                        
                        self.__find_and_set_templated_type(return_type, i.parent_namespace, types)
            
        # Stage 5 process (even further template processing)
        # Set full_safe_type_name for each template instance
        for t in self.templates.values():
            for i in t.instances:
                full_safe_type_name = t.definition.full_safe_type_name

                for a in i.arguments:
                    full_safe_type_name += '_' + a.full_safe_type_name
                
                i.full_safe_type_name = full_safe_type_name

        log_file.flush()
        log_file.close()