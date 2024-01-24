from __future__ import annotations

from dataclasses import dataclass, field
from typing import List, Union


@dataclass
class EnumFieldMetadata:
    """ Describes a field inside an enum. """


    name: str
    value: str | None
    doc_comments: List[str] | None = None


@dataclass
class EnumMetadata:
    """ Describes an enum and its fields. """


    header_file: str
    start_line: int
    end_line: int
    namespace: str
    name: str
    full_safe_type_name: str
    base: str | None
    fields: List[EnumFieldMetadata]
    is_flags: bool
    is_nested_type: bool = False
    doc_comments: List[str] | None = None


@dataclass
class FieldMetadata:
    """ Describes a class or struct field. """


    parent_type: Union[ClassMetadata, StructMetadata] = field(repr=False)
    namespace: str
    name: str
    type: TypeMetadata
    is_static: bool
    unique_getter_name: str = None
    unique_setter_name: str = None


@dataclass
class StructMetadata:
    """ Describes a struct and its fields. """


    header_file: str
    start_line: int
    namespace: str
    name: str
    base: str | None
    end_line: int = -1
    fields: List[FieldMetadata] = None
    is_nested_type: bool = False
    doc_comments: List[str] = None


@dataclass
class TemplateArgumentMetadata:
    """ Describes the type of a template instance argument """


    type: TypeMetadata
    parameter_name: str = None
    safe_type_name: str = None
    full_safe_type_name: str = None
    is_last: bool = False


@dataclass
class TypeMetadata:
    """ Describes the type of a parameter, field, or return value. """


    namespace: str | None
    name: str
    is_pointer: bool = False
    is_pointer_pointer: bool = False
    is_reference: bool = False
    is_pointer_or_reference: bool = False
    is_const: bool = False
    is_string: bool = False
    is_function_signature: bool = False
    function_signature: FunctionMetadata | None = None
    is_template: bool = False
    template_name: str | None = None
    template_safe_type_name: str | None = None
    template_arguments: List[TemplateArgumentMetadata] | None = None
    is_optional: bool = False
    is_class: bool = False
    is_interface: bool = False
    is_class_or_interface: bool = False
    is_enum: bool = False
    is_inline_forward: bool = False
    is_template_argument: bool = False
    is_primitive: bool = False


@dataclass
class ParameterMetadata:
    """ Describes a function parameter. """


    name: str
    type: TypeMetadata
    is_out: bool
    is_in_out: bool
    is_last: bool = False


@dataclass
class FunctionMetadata:
    """ Describes a function. """


    header_file: str | None
    start_line: int
    end_line: int
    namespace: str | None
    name: str | None
    return_type: TypeMetadata | None
    has_return: bool
    has_parameters: bool
    parameters: List[ParameterMetadata] | None
    parent_class: ClassMetadata = field(repr=False, default=None)
    unique_name: str = None
    is_explicit_converter: bool = False
    is_static: bool = False
    is_virtual: bool = False
    is_override: bool = False
    is_interface_implementation: bool = False
    is_constructor: bool = False
    is_destructor: bool = False
    is_const: bool = False
    is_async_result: bool = False
    is_async_result_with_progress: bool = False
    is_event: bool = False
    is_operator_overload: bool = False
    is_index_operator: bool = False
    is_equal_operator: bool = False
    is_notequal_operator: bool = False
    doc_comments: List[str] = None
    is_private: bool = False
    is_deprecated: bool = False
    deprecation_message: str | None = None


@dataclass
class ClassMetadata:
    """ Describes a class, its fields, and its methods. """


    header_file: str
    start_line: int
    end_line: int
    namespace: str
    name: str
    inherits_from: List[TypeMetadata]
    fields: List[FieldMetadata]
    methods: List[FunctionMetadata]
    full_safe_type_name: str
    should_dispose: bool
    base: TypeMetadata = None
    has_base_type: bool = False
    interfaces: List[ClassInterfaceMetadata] = None
    has_interfaces: bool = False
    is_pure_virtual: bool = False
    is_static: bool = False
    is_nested_type: bool = False
    has_nested_types: bool = False
    is_template_instance: bool = False
    template_name: str = None
    template_arguments: List[TemplateArgumentMetadata] = None
    doc_comments: List[str] = None


@dataclass
class ClassInterfaceMetadata:
    """ Describes an interface that a class implements """


    name: str
    type: TypeMetadata
    is_last: bool = False


@dataclass
class InterfaceMetadata:
    """ Describes an interface (pure virtual class) and its methods. """


    header_file: str
    start_line: int
    end_line: int
    namespace: str
    name: str
    methods: List[FunctionMetadata]
    full_safe_type_name: str
    interfaces: List[ClassInterfaceMetadata] | None = None
    has_interfaces: bool = False
    is_nested_type: bool = False
    doc_comments: List[str] = None


@dataclass
class TypedefMetadata:
    """ Describes a typedef. """


    namespace: str
    name: str
    type: TypeMetadata
    doc_comments: List[str] = None


@dataclass
class TemplateInstanceMetadata:
    """ Describes a template instance. """


    parent_namespace: str
    template_parameters: List[TemplateParameterMetadata]
    arguments: List[TemplateArgumentMetadata]
    full_safe_type_name: str = None

    def get_argument_for_parameter(self, parameter_name: str) -> TemplateArgumentMetadata:
        for i in range(len(self.template_parameters)):
            if self.template_parameters[i].name == parameter_name:
                return self.arguments[i]
        
        return None

    def argument_name(self, parameter: str, render) -> str:
        rendered = render(parameter)
        arg = self.get_argument_for_parameter(rendered)

        if arg == None:
            return render('{{> Type }}')

        if getattr(arg.type, 'translated_namespace', None) != None:
            return f"{getattr(arg.type, 'translated_namespace')}.{arg.type.name}"

        if arg.type.namespace != None and arg.type.namespace != "":
            return f"{arg.type.namespace}::{arg.type.name}"

        return arg.type.name
    
    def get_template_argument(self, parameter: str, render) -> str:
        block_begin = parameter.index('{{# parameter_name}}') + len('{{# parameter_name}}')
        block_end = parameter.index('{{/ parameter_name}}')
        parameter_name = parameter[block_begin:block_end]
        block_end += len('{{/ parameter_name}}')
        body = parameter[block_end:]

        rendered_parameter_name = render(parameter_name)
        arg = self.get_argument_for_parameter(rendered_parameter_name)

        return render(f"{{{{# _template_argument__ }}}}{ body }{{{{/ _template_argument__ }}}}", { '_template_argument__': arg })
    
    def _get_template_argument(self, parameter: str, render) -> str:
        return self.get_template_argument(parameter, render)


@dataclass
class TemplateParameterMetadata:
    """ Describes a template parameter. """


    name: str
    is_last: bool = False


@dataclass
class TemplateMetadata:
    """ Describes a template class. """


    definition: ClassMetadata
    template_parameters: List[TemplateParameterMetadata]
    instances: List[TemplateInstanceMetadata] = None
