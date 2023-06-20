#!python3

from copy import deepcopy
from pathlib import Path
from typing import Dict

import chevron
import os
import subprocess

from Config import config
from MetadataTypes import EnumMetadata, StructMetadata, FunctionMetadata, ClassMetadata, TemplateMetadata, InterfaceMetadata
from Parser import read_whole_file


TEMPLATE_DIRECTORY = config['template_directory'] + 'C/'
PARTIALS_DIRECTORY = TEMPLATE_DIRECTORY + 'Partials/'
OUTPUT_DIRECTORY = config['output_directory'] + 'C/'


class CWrapperGenerator:    
    def generate(self, enums: Dict[str, EnumMetadata], structs: Dict[str, StructMetadata], functions: Dict[str, FunctionMetadata],
        classes: Dict[str, ClassMetadata], templates: Dict[str, TemplateMetadata], interfaces: Dict[str, InterfaceMetadata]
    ) -> None:        
        # Create output directory if it doesn't exist
        Path(OUTPUT_DIRECTORY).mkdir(parents=True, exist_ok=True)
        
        includes = [f.header_file for f in functions.values()] + [c.header_file for c in classes.values()]
        # Remove duplicate entries by converting to dict
        includes = list(dict.fromkeys(includes))
        
        template = read_whole_file(TEMPLATE_DIRECTORY + 'GeneratedWrapper.mustache')
        
        with open(f"{OUTPUT_DIRECTORY}generated_wrapper.h", 'w') as f:
            print(
                chevron.render(
                    template, {
                        'includes': includes,
                        'functions': list(functions.values()),
                        'classes': list(classes.values()),
                        'templates': list(templates.values()),
                        'extra_data': config
                        },
                    PARTIALS_DIRECTORY,
                    warn=True
                ),
                file=f
            )
        
        subprocess.run(f"\"clang-format\" -i \"{OUTPUT_DIRECTORY}generated_wrapper.h\"", shell=True)
