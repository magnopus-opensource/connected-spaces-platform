from __future__ import annotations

from copy import deepcopy
from dataclasses import dataclass, field
from enum import auto, IntEnum, IntFlag
from typing import List, Dict

from jinja2 import Environment, FileSystemLoader, Template

from Config import config
from MetadataTypes import \
    ClassMetadata, TemplateArgumentMetadata, TemplateMetadata, \
    FunctionMetadata as OldFunctionMetadata, \
    TypeMetadata as OldTypeMetadata, \
    ParameterMetadata as OldParameterMetadata


@dataclass
class NamespaceMetadata:
    """ Describes a type's namespace. """


    parts: List[str]

    def __repr__(self) -> str:
        return '::'.join(self.parts)


class ParameterModifier(IntEnum):
    NONE = 0
    OUT = auto()
    IN_OUT = auto()


@dataclass
class ParameterMetadata:
    """ Describes a function parameter. """


    name: str
    type: TypeMetadata
    modifier: ParameterModifier
    is_last: bool = False


class PointerType(IntEnum):
    NONE = 0
    POINTER = auto()
    REFERENCE = auto()
    POINTER_POINTER = auto()

    def is_pointer_or_reference(self) -> bool:
        return self == PointerType.POINTER or self == PointerType.REFERENCE


class TypeModifiers(IntFlag):
    NONE = 0
    CONST = 1
    OPTIONAL = 2


class TypeClassification(IntEnum):
    NONE = 0
    PRIMITIVE = auto()
    STRING = auto()
    ENUM = auto()
    CLASS = auto()
    INTERFACE = auto()
    TEMPLATE = auto()
    FUNCTION_POINTER = auto()


@dataclass
class PrimitiveTypeInfo:
    is_signed: bool
    is_floating_point: bool
    width: int


@dataclass
class TypeMetadata:
    """ Describes the type of a parameter, field, or return value. """


    namespace: NamespaceMetadata
    name: str
    pointer_type: PointerType
    modifiers: TypeModifiers
    classification: TypeClassification
    primitive_info: PrimitiveTypeInfo | None = None
    function_signature: FunctionMetadata | None = None
    template: TemplateMetadata | None = None
    template_arguments: List[TemplateArgumentMetadata] | None = None
    is_inline_forward: bool = False
    is_template_argument: bool = False


class FunctionModifiers(IntFlag):
    NONE = 0
    STATIC = 1
    VIRTUAL = 2
    OVERRIDE = 4
    CONST = 8


class FunctionClassification(IntEnum):
    DEFAULT = 0
    CONSTRUCTOR = auto()
    PRIVATE_CONSTRUCTOR = auto()
    DESTRUCTOR = auto()
    PRIVATE_DESTRUCTOR = auto()
    EXPLICIT_CONVERTOR = auto()
    OPERATOR_OVERLOAD = auto()
    ASYNC_RESULT = auto()
    ASYNC_RESULT_WITH_PROGRESS = auto()
    EVENT = auto()


class OperatorOverloadType(IntEnum):
    NONE = 0
    INDEX = auto()
    EQUAL = auto()
    NOT_EQUAL = auto()


@dataclass
class FunctionMetadata:
    """ Describes a function. """


    header_file: str
    start_line: int
    end_line: int
    namespace: NamespaceMetadata
    name: str
    modifiers: FunctionModifiers
    classification: FunctionClassification
    return_type: TypeMetadata | None
    parameters: List[ParameterMetadata] | None
    parent_class: ClassMetadata | None = field(repr=False, default=None)
    unique_name: str = ''
    is_interface_implementation: bool = False
    operator_overload_type: OperatorOverloadType | None = None
    doc_comments: List[str] | None = None


def typescriptify_namespace(value: NamespaceMetadata, separator: str = '.') -> str:
    namespace = ''

    for i, p in enumerate(value.parts):
        part = ''

        if p in TypeScriptWrapperGenerator_Jinja2.NAMESPACE_TRANSLATIONS:
            part = separator.join(TypeScriptWrapperGenerator_Jinja2.NAMESPACE_TRANSLATIONS[p])
        else:
            part = p
        
        namespace = part if i == 0 else f"{namespace}{separator}{part}"
    
    return namespace


def typescriptify(value: NamespaceMetadata | str) -> str:
    if isinstance(value, NamespaceMetadata):
        return typescriptify_namespace(value)
    elif isinstance(value, str):
        # Assume value is a name
        return value[0].lower() + value[1:]
    
    return '[[-- ERROR TYPESCRIPTIFYING VALUE --]]'


def trim_prefix(name: str) -> str:
    if name.startswith('Out') and name[3].isupper():
        return name[3].lower() + name[4:]
    elif name.startswith('InOut') and name[5].isupper():
        return name[5].lower() + name[6:]

    return name


class TypeScriptWrapperGenerator_Jinja2:
    NAMESPACE_TRANSLATIONS = {
        'csp': '',
        'csp::memory': 'Memory',
        'csp::multiplayer': 'Multiplayer',
        'csp::services': 'Services',
        'csp::systems': 'Systems',
        'csp::web': 'Web'
    }

    metadata_templates: Dict[str, TemplateMetadata]
    function_template: Template

    def __upgrade_parameter_metadata(self, parameter: OldParameterMetadata) -> ParameterMetadata:
        type = self.__upgrade_type_metadata(parameter.type)

        modifier = ParameterModifier.NONE

        if parameter.is_out:
            modifier = ParameterModifier.OUT
        elif parameter.is_in_out:
            modifier = ParameterModifier.IN_OUT

        return ParameterMetadata(
            name=parameter.name,
            type=type,
            modifier=modifier,
            is_last=parameter.is_last
        )


    def __upgrade_type_metadata(self, type: OldTypeMetadata) -> TypeMetadata | None:
        if type is None:
            return None
        
        name = type.name

        namespace_parts: List[str] = []

        if type.namespace is not None:
            namespace_parts = type.namespace.split('::')

        namespace = NamespaceMetadata(namespace_parts)

        pointer_type = PointerType.NONE

        if type.is_pointer_pointer:
            pointer_type = PointerType.POINTER_POINTER
        elif type.is_pointer:
            pointer_type = PointerType.POINTER
        elif type.is_reference:
            pointer_type = PointerType.REFERENCE

        modifiers = TypeModifiers.NONE

        if type.is_const:
            modifiers |= TypeModifiers.CONST
        
        if type.is_optional:
            modifiers |= TypeModifiers.OPTIONAL
        
        classification: TypeClassification

        if type.name == 'void':
            classification = TypeClassification.NONE
        elif type.is_string:
            classification = TypeClassification.STRING
        elif type.is_enum:
            classification = TypeClassification.ENUM
        elif type.is_class:
            classification = TypeClassification.CLASS
        elif type.is_interface:
            classification = TypeClassification.INTERFACE
        elif type.is_template:
            classification = TypeClassification.TEMPLATE
        elif type.is_function_signature:
            classification = TypeClassification.FUNCTION_POINTER
        else:
            classification = TypeClassification.PRIMITIVE
        
        primitive_info: PrimitiveTypeInfo | None = None

        if classification == TypeClassification.PRIMITIVE:
            is_signed = False
            is_floating_point = False
            width = 0

            # Simplify type names
            if name == 'int8_t' or name == 'signed char':
                name = 'int8_t'
                is_signed = True
                width = 8
            elif name == 'uint8_t' or name == 'unsigned char':
                name = 'uint8_t'
                width = 8
            elif name == 'int16_t' or name == 'short' or name == 'signed short' or name == 'short int' or name == 'unsigned short int':
                name = 'int16_t'
                is_signed = True
                width = 16
            elif name == 'uint16_t' or name == 'unsigned short' or name == 'unsigned short int':
                name = 'uint16_t'
                width = 16
            elif name == 'int32_t' or name == 'int' or name == 'signed int' or name == 'long' or name == 'signed long' or name == 'long int' or name == 'signed long int':
                name = 'int32_t'
                is_signed = True
                width = 32
            elif name == 'uint32_t' or name == 'unsigned int' or name == 'unsigned long' or name == 'unsigned long int':
                name = 'uint32_t'
                width = 32
            elif name == 'int64_t' or name == 'long long' or name == 'signed long long' or name == 'long long int' or name == 'signed long long int':
                name = 'int64_t'
                is_signed = True
                width = 64
            elif name == 'uint64_t' or name == 'unsigned long long' or name == 'unsigned long long int':
                name = 'uint64_t'
                width = 64
            elif name == 'intptr_t':
                is_signed = True
                width = 64
            elif name == 'uintptr_t' or name == 'size_t':
                width = 64
            elif name == 'float':
                is_floating_point = True
                width = 32
            elif name == 'double':
                is_floating_point = True
                width = 64

            primitive_info = PrimitiveTypeInfo(
                is_signed=is_signed,
                is_floating_point=is_floating_point,
                width=width,
            )
        
        function_signature: FunctionMetadata | None = None

        if type.function_signature is not None:
            function_signature = self.__upgrade_function_metadata(type.function_signature)
        
        template: TemplateMetadata | None = None

        if type.is_template:
            template = self.metadata_templates[type.template_name]

        return TypeMetadata(
            namespace=namespace,
            name=name,
            pointer_type=pointer_type,
            modifiers=modifiers,
            classification=classification,
            primitive_info=primitive_info,
            function_signature=function_signature,
            template=template,
            template_arguments=type.template_arguments,
            is_inline_forward=type.is_inline_forward,
            is_template_argument=type.is_template_argument
        )


    def __upgrade_function_metadata(self, function: OldFunctionMetadata) -> FunctionMetadata:
        namespace = NamespaceMetadata(function.namespace.split('::'))
        modifiers = FunctionModifiers.NONE

        if function.is_static:
            modifiers |= FunctionModifiers.STATIC
        
        if function.is_virtual:
            modifiers |= FunctionModifiers.VIRTUAL
        
        if function.is_override:
            modifiers |= FunctionModifiers.OVERRIDE
        
        if function.is_const:
            modifiers |= FunctionModifiers.CONST

        classification = FunctionClassification.DEFAULT

        if function.is_explicit_converter:
            classification = FunctionClassification.EXPLICIT_CONVERTOR
        elif function.is_constructor:
            if function.is_private:
                classification = FunctionClassification.PRIVATE_CONSTRUCTOR
            else:
                classification = FunctionClassification.CONSTRUCTOR
        elif function.is_destructor:
            if function.is_private:
                classification = FunctionClassification.PRIVATE_DESTRUCTOR
            else:
                classification = FunctionClassification.DESTRUCTOR
        elif function.is_operator_overload:
            classification = FunctionClassification.OPERATOR_OVERLOAD
        elif function.is_async_result:
            classification = FunctionClassification.ASYNC_RESULT
        elif function.is_async_result_with_progress:
            classification = FunctionClassification.ASYNC_RESULT_WITH_PROGRESS
        elif function.is_event:
            classification = FunctionClassification.EVENT
        
        return_type = self.__upgrade_type_metadata(function.return_type)
        parameters: List[ParameterMetadata] = []

        for p in function.parameters:
            parameter = self.__upgrade_parameter_metadata(p)
            parameters.append(parameter)

        operator_overload_type: OperatorOverloadType | None = None

        if classification == FunctionClassification.OPERATOR_OVERLOAD:
            if function.is_index_operator:
                operator_overload_type = OperatorOverloadType.INDEX
            elif function.is_equal_operator:
                operator_overload_type = OperatorOverloadType.EQUAL
            elif function.is_notequal_operator:
                operator_overload_type = OperatorOverloadType.NOT_EQUAL

        return FunctionMetadata(
            header_file=function.header_file,
            start_line=function.start_line,
            end_line=function.end_line,
            namespace=namespace,
            name=function.name,
            modifiers=modifiers,
            classification=classification,
            return_type=return_type,
            parameters=parameters,
            parent_class=function.parent_class,
            unique_name=function.unique_name,
            is_interface_implementation=function.is_interface_implementation,
            operator_overload_type=operator_overload_type,
            doc_comments=function.doc_comments
        )

    def generate(self, functions: Dict[str, OldFunctionMetadata], templates: Dict[str, TemplateMetadata]) -> List[str]:
        # # Deepcopy all metadata so we don't modify the original data for any wrapper generator classes that get called after this one
        self.metadata_templates = deepcopy(templates)

        # Upgrade metadata types
        _functions: List[FunctionMetadata] = []

        for f in functions.values():
            _function = self.__upgrade_function_metadata(f)
            _functions.append(_function)

        # Initialise Jinja environment and add custom filters
        env = Environment(
            loader=FileSystemLoader('Templates/TypeScript'),
            extensions=['jinja2_workarounds.MultiLineInclude']
        )

        def register_type(type, _env = env):
            _env.globals[type.__name__] = type

        register_type(TypeClassification)
        register_type(TypeModifiers)
        register_type(PointerType)
        register_type(ParameterModifier)

        def register_filter(filter, _env = env):
            _env.filters[filter.__name__] = filter

        register_filter(typescriptify)
        register_filter(trim_prefix)

        # Render global functions
        function_template = env.get_template('global_function.ts.jinja2')
        rendered_functions = []

        for f in _functions:
            rendered_functions.append(function_template.render(this=f))

        return rendered_functions