from __future__ import annotations

from copy import deepcopy
from typing import List, Dict, Any

from jinja2 import Environment, FileSystemLoader, Template

from Config import config
from MetadataTypes import \
    TemplateMetadata, \
    FunctionMetadata as OldFunctionMetadata, \
    ClassMetadata as OldClassMetadata
from MetadataTypes_Jinja2 import \
    NamespaceMetadata, \
    FunctionMetadata, ParameterModifier, \
    TypeClassification, TypeModifiers, PointerType, \
    upgrade_function_metadata


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
        'csp::systems': 'Systems',
        'csp::web': 'Web'
    }

    function_template: Template

    def generate(self, functions: Dict[str, OldFunctionMetadata], classes: Dict[str, OldClassMetadata], templates: Dict[str, TemplateMetadata]) -> Dict[str, Any]:
        # Deepcopy all metadata so we don't modify the original data for any wrapper generator classes that get called after this one
        templates = deepcopy(templates)

        # Upgrade metadata types
        _functions: List[FunctionMetadata] = []

        for f in functions.values():
            _function = upgrade_function_metadata(f, templates)
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

        return {
            'global_functions': rendered_functions
        }