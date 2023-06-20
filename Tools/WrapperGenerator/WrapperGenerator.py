#!/usr/bin/env python3

import os
import sys
import argparse
from glob import glob

from CWrapperGenerator import CWrapperGenerator
from CSharpWrapperGenerator import CSharpWrapperGenerator
from TypeScriptWrapperGenerator import TypeScriptWrapperGenerator
from Parser import Parser

from Config import config


os.chdir(os.path.dirname(sys.argv[0]))


arg_parser = argparse.ArgumentParser()
arg_parser.add_argument('--generate_csharp', action='store_true')
arg_parser.add_argument('--generate_typescript', action='store_true')
arg_parser.add_argument('--no_generate_c', action='store_true')
arg_parser.add_argument('--include_directory', action='store')
args = arg_parser.parse_args()

# Use the passed include directory value if one exists
if args.include_directory is not None:
    config['public_include_directory'] = f"{ os.path.dirname(os.path.realpath(__file__)) }/{ args.include_directory }"

print('Include Directory: ' + config['public_include_directory'])
print('Output Folder: ' + config['output_directory'])

# Get all includes under include folder
results = [y for x in os.walk(config['public_include_directory']) for y in glob(os.path.join(x[0], '*.h'))] 
print('Header Count: ' + str(len(results)))

parser = Parser()

# For every header parse exported functionality by macro.
# We export structs, classes, interfaces(pure virtual classes), global functions, enums, and typedefs
try:
    parser.parse(results)
except SystemExit as e:
    os._exit(e.code)

enums = parser.enums
structs = parser.structs
functions = parser.functions
classes = parser.classes
templates = parser.templates
interfaces = parser.interfaces


if not args.no_generate_c:
    print('Generating C interface')
    CWrapperGenerator().generate(enums, structs, functions, classes, templates, interfaces)

if args.generate_csharp:
    print('Generating C# wrapper')
    CSharpWrapperGenerator().generate(enums, structs, functions, classes, templates, interfaces)

if args.generate_typescript:
    print('Generating TypeScript wrapper')
    TypeScriptWrapperGenerator().generate(enums, structs, functions, classes, templates, interfaces)