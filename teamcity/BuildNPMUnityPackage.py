import os
import argparse
import shutil
import subprocess
import json
import chevron
import binascii

from yaml import load, dump

try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper

from distutils.dir_util import copy_tree


class OlympusFoundationPyError(Exception): pass
class FileHandlingError(OlympusFoundationPyError): pass


def get_arguments_commandline():
    parser = argparse.ArgumentParser(description='Build the Olympus Foundation Release for NPM.')

    parser.add_argument('--version',
                        help="Enter the version of the Build Olympus Foundation, Semantic Versioning Only.", 
                        default="0.0.0")
    parser.add_argument('--name',
                        help="Enter the name of the Build Olympus Foundation package.", 
                        default="@magnopus/olympus-foundation")
    parser.add_argument('--display_name',
                        help="Enter the display name of the Build Olympus Foundation package.", 
                        default="Olympus Foundation")
    parser.add_argument('--assembly_name',
                        help="Enter the assembly name of the Olympus Foundation package.",
                        default="Olympus.Foundation")
    parser.add_argument('--relative_destination_path',
                        help="Enter the relative path from root/teamcity for the libraries to be copied to.", 
                        default="Library\\Binaries\\package")
    parser.add_argument('--relative_windows_path',
                        help="Enter the relative path from root/teamcity for windows.",
                        default=None)
    parser.add_argument('--relative_mac_path',
                        help="Enter the relative path from root/teamcity for mac.",
                        default=None)
    parser.add_argument('--relative_android_path',
                        help="Enter the relative path from root/teamcity for android.",
                        default=None)
    parser.add_argument('--relative_ios_path',
                        help="Enter the relative path from root/teamcity for ios.",
                        default=None)
    parser.add_argument('--relative_linux_path',
                        help="Enter the relative path from root/teamcity for linux.",
                        default=None)
    parser.add_argument('--relative_csharp_source_path',
                        help="Enter the relative path from root/teamcity for the C# wrapper source.",
                        default=None)
    parser.add_argument('--unity_version',
                        help="Enter the version of Unity supported, e.g. 2021.1.", 
                        default="2020.1")
    parser.add_argument('--license',
                        help="Enter the license required for the Olympus Foundation package.", 
                        default="UNLICENSED")
    parser.add_argument('--dependencies',
                        help="Enter the dependencies required for the Olympus Foundation package.", 
                        default=None)
    parser.add_argument('--description',
                        help="This package provides the DLL's required to interface with the Olympus project servers.", 
                        default="Exposes Olympus functionality via the Foundation API")
    parser.add_argument('--registry',
                        help="This is the upstream location of the package.", 
                        default="http://packages.magnopus.local:8081/artifactory/api/npm/unitynpm/")
    parser.add_argument('--generation_folder',
                        help="This is the package generation location.", 
                        default="package_gen")
    parser.add_argument('--release_mode',
                        help="NPM release command, pack and publish are available.", 
                        default="pack")
    parser.add_argument('--template_file',
                        help="The metafile template file to enable settings injections per platform",
                        default="templates/metafile_template.mustache")
    parser.add_argument('--template_dll_file',
                        help="The dll metafile template file to enable settings injections per platform",
                        default="templates/dll_template.mustache")
    parser.add_argument('--template_default_file',
                        help="The default metafile template file to enable settings injections per platform",
                        default="templates/default_template.mustache")
    parser.add_argument('--template_package_file',
                        help="The package metafile template file to enable settings injections per platform",
                        default="templates/package_template.mustache")
    parser.add_argument('--template_source_file',
                        help="The source metafile template file to enable settings injections per platform",
                        default="templates/source_template.mustache")
    parser.add_argument('--template_asmdef_file',
                        help="The asmdef metafile template file to enable settings injections per platform",
                        default="templates/asmdef_template.mustache")
    parser.add_argument('--unity_platform_settings_file',
                        help="The metafile template file to inject the correct settings into the template",
                        default="templates/unity_platform_settings.json")
    parser.add_argument('--platform_file',
                        help="The platform metafile file to inject into the metafiles to allow platform interaction",
                        default="templates/platform_files.json")
                     
    args = parser.parse_args()

    return args


def create_output_path(output_path):
    try:
        if not os.path.exists(os.path.dirname(output_path)):
            os.mkdir(os.path.dirname(output_path))

        if not os.path.exists(output_path):
            os.mkdir(output_path)

        if not os.path.exists(os.path.join(output_path, 'Source')):
            os.mkdir(os.path.join(output_path, 'Source'))
    except IOError as e:
        print("Issue changing folder, Error code (%s)." % e)

    try:
        print("Attempting to remove folder " + output_path + "\n")
        shutil.rmtree(os.path.dirname(output_path))
        print("Removed folder successfully!")
    except IOError as e:
        print("Issue removing folder, the folder may not have existed, Error code (%s)." % e)

    try:
        print("Attempting to create folder " + output_path + "\n")
        os.mkdir(os.path.dirname(output_path))
        os.mkdir(output_path)
        os.mkdir(os.path.join(output_path, 'Source'))
        print("Created folder successfully!")
    except IOError as e:
        print("Issue creating folder, Error code (%s)." % e)
        raise FileHandlingError("Issue creating folder, Error code (%s)." % e)


def copy_packages_in(input_args, output_path):
    input_paths = []
    windows_items = None
    mac_items = None
    android_items = None
    ios_items = None
    linux_items = None

    windows_file_extensions = ('.dll', '.pdb', '.md')
    mac_file_extensions = ('.dylib', '.dynlib')
    ios_file_extensions = tuple('.a')
    unix_file_extensions = tuple('.so')
    all_file_extensions = windows_file_extensions + mac_file_extensions + ios_file_extensions + unix_file_extensions

    if(input_args.relative_windows_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_windows_path))
        windows_items = os.listdir(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_windows_path))
        windows_items = [x for x in windows_items if x.endswith(windows_file_extensions)]
        # Remove compiled C# wrapper DLL
        windows_items = [x for x in windows_items if not "Olympus.Foundation.dll" in x]

    if(input_args.relative_mac_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_mac_path))
        mac_items = os.listdir(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_mac_path))
        mac_items = [x for x in mac_items if x.endswith(mac_file_extensions) and 'dSym' not in x]

    if(input_args.relative_android_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_android_path))
        android_items = os.listdir(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_android_path))
        android_items = [x for x in android_items if x.endswith(unix_file_extensions)]

    if(input_args.relative_ios_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_ios_path))
        ios_items = os.listdir(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_ios_path))
        ios_items = [x for x in ios_items if x.endswith(ios_file_extensions) and 'dSym' not in x]

    if(input_args.relative_linux_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_linux_path))
        linux_items = os.listdir(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_linux_path))
        linux_items = [x for x in linux_items if x.endswith(unix_file_extensions)]
    
    # Copy in .npmrc file
    shutil.copy2(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), "Library/Binaries/unity_npmrc", ".npmrc"), output_path)

    for binary_folder in input_paths:
        for item in os.listdir(binary_folder):
            if not os.path.isdir(item):
                if item.endswith(all_file_extensions) and not 'dSYM' in item and not 'Olympus.Foundation.dll' in item:
                    shutil.copy2(os.path.join(binary_folder, item), output_path)

    if(input_paths):
        return True, windows_items, mac_items, android_items, ios_items, linux_items
    else:
        return False, windows_items, mac_items, android_items, ios_items, linux_items


def create_package_file(input_args, output_path):
    print("Creating package file...")
    f = open(os.path.join(output_path, "package.json"), "x")
    f.writelines([
        '{\n',
       f'  "name": "{input_args.name}",\n',
       f'  "displayName": "{input_args.display_name}",\n',
       f'  "unity": "{input_args.unity_version}",\n',
       f'  "version": "{input_args.version}",\n',
       f'  "description": "{input_args.description}",\n',
       f'  "license": "{input_args.license}",\n',
        '  "dependencies": {\n'
        '  },\n',
        '  "publishConfig": {\n',
       f'    "registry": "{input_args.registry}/@magnopus"\n',
        '  },\n'
        '  "repository": "https://github.com/magnopus/olympus-foundation"'
        '}\n'
    ])
    f.close()
    print("Created package file successfully!")


def create_initial_template(input_args, teamcity_folder, generation_folder):
    data = None
    output = None
    bytes_in_guid = 16

    print("Creating initial template...")

    for root, dirs, files in os.walk(generation_folder):
        # Generate default meta files for folders
        for dir in dirs:
            guid_generated = binascii.b2a_hex(os.urandom(bytes_in_guid))

            with open(os.path.join(teamcity_folder, input_args.template_default_file), 'r') as file_in:
                data = file_in.read()
                output = chevron.render(data, {'asset_guid': guid_generated.decode('utf-8')})

            with open(os.path.join(generation_folder, f"{ root }/{ dir }.meta"), "w+") as f:
                f.write(output)

        # Generate meta files for files based on extension
        for file in files:
            guid_generated = binascii.b2a_hex(os.urandom(bytes_in_guid))

            if file.endswith(".dll"):
                with open(os.path.join(teamcity_folder, input_args.template_dll_file), 'r') as file_in:
                    data = file_in.read()
                    output = chevron.render(data, {'asset_guid': guid_generated.decode('utf-8'), 'importer_specified': 'PluginImporter'})
            elif file.endswith(".asmdef"):
                with open(os.path.join(teamcity_folder, input_args.template_asmdef_file), 'r') as file_in:
                    data = file_in.read()
                    output = chevron.render(data, {'asset_guid': guid_generated.decode('utf-8')})
            elif file.endswith(".cs"):
                with open(os.path.join(teamcity_folder, input_args.template_source_file), 'r') as file_in:
                    data = file_in.read()
                    output = chevron.render(data, {'asset_guid': guid_generated.decode('utf-8')})
            elif "package.json" in file:
                with open(os.path.join(teamcity_folder, input_args.template_package_file), 'r') as file_in:
                    data = file_in.read()
                    output = chevron.render(data, {'asset_guid': guid_generated.decode('utf-8')})
            else:
                with open(os.path.join(teamcity_folder, input_args.template_default_file), 'r') as file_in:
                    data = file_in.read()
                    output = chevron.render(data, {'asset_guid': guid_generated.decode('utf-8')})

            with open(os.path.join(generation_folder, f"{ root }/{ file }.meta"), "w+") as f:
                f.write(output)
    
    print("Created initial template successfully!")
    

def load_and_match_platform_files(input_args, teamcity_folder):
    all_items = None

    with open(os.path.join(teamcity_folder, input_args.platform_file)) as f:
        data = json.load(f)
        all_items = data['all']
        
    return all_items


def find_file_section_to_inject(file_path, appendix):
    data = None

    with open(file_path, "r") as f:
        data = load(f, Loader=Loader)

    data_to_insert = load(appendix, Loader=Loader)
    
    if 'PluginImporter' in data:
        pi = data['PluginImporter']

        if 'platformData' in pi:
            pi['platformData'] = data_to_insert['platformData']
            
            with open(file_path, "w") as f:
                f.write(dump(data, Dumper=Dumper, sort_keys=False))


def get_template(input_args, platform, teamcity_folder):
    data = None

    with open(os.path.join(teamcity_folder, input_args.unity_platform_settings_file), 'r') as f:
        data = json.load(f)

    if data:
        with open(os.path.join(teamcity_folder, input_args.template_file), 'r') as f:
            if(platform == "all"):
                return chevron.render(f, data['all'])
            elif(platform == "android"):
                return chevron.render(f, data['android'])
            elif(platform == "ios"):
                return chevron.render(f, data['ios'])
            elif(platform == "windows"):
                return chevron.render(f, data['windows'])
            elif(platform == "mac"):
                return chevron.render(f, data['mac'])
            elif(platform == "linux"):
                return chevron.render(f, data['linux'])


def inject_template(input_args, teamcity_folder, file_list, generation_folder, platform):
    appendix_generated = get_template(input_args, platform, teamcity_folder)

    if appendix_generated:
        for file in file_list:
            find_file_section_to_inject(os.path.join(generation_folder, file + ".meta"), appendix_generated)


def inject_metafile_data(input_args, generation_folder, teamcity_folder, windows_items, mac_items, android_items, ios_items, linux_items):
    print("Injecting metafile data from template...")
    files_processed = os.listdir(generation_folder)
    shared_items = load_and_match_platform_files(input_args, teamcity_folder)

    # matched items for all
    all_shared_items = set(shared_items) & set(files_processed)
    inject_template(input_args, teamcity_folder, all_shared_items, generation_folder, "all")

    if(input_args.relative_windows_path):
        for item in all_shared_items:
            if item in windows_items:
                windows_items.remove(item)

        inject_template(input_args, teamcity_folder, windows_items, generation_folder, "windows")

    if(input_args.relative_mac_path):
        for item in all_shared_items:
            if item in mac_items:
                mac_items.remove(item)

        inject_template(input_args, teamcity_folder, mac_items, generation_folder, "mac")

    if(input_args.relative_android_path):
        for item in all_shared_items:
            if item in android_items:
                android_items.remove(item)

        inject_template(input_args, teamcity_folder, android_items, generation_folder, "android")

    if(input_args.relative_ios_path):
        for item in all_shared_items:
            if item in ios_items:
                ios_items.remove(item)

        inject_template(input_args, teamcity_folder, ios_items, generation_folder, "ios")

    if(input_args.relative_linux_path):
        for item in all_shared_items:
            if item in linux_items:
                linux_items.remove(item)

        inject_template(input_args, teamcity_folder, linux_items, generation_folder, "linux")

    print("Injected metafile data from template successfully!")


def generate_final_package(input_args, generation_folder):
    print("Attempting to generate and publish npm package...")
    os.chdir(generation_folder)
    
    subprocess.call("docker run --env NODE_OPTIONS=\"--max-old-space-size=8192\" --rm -w /src -v " + generation_folder + ":/src node:lts-bullseye-slim npm install . --userconfig=.npmrc && npm " + input_args.release_mode + f" --\"@magnopus:registry={input_args.registry}\"", shell=True)
        
    print("Published npm package!")


def copy_wrapper_source(source_folder, generation_folder):
    copy_tree(source_folder, os.path.join(generation_folder, 'Source'))


def create_wrapper_asmdef_file(input_args, output_path):
    print("Creating C# source asmdef file...")
    f = open(os.path.join(output_path, f'{ input_args.assembly_name }.asmdef'), "x")
    f.writelines([
        '{\n',
       f'  "name": "{ input_args.assembly_name }",\n',
       f'  "rootNamespace": "{ input_args.assembly_name }",\n',
        '  "references": [],\n',
        '  "includePlatforms": [],\n',
        '  "excludePlatforms": [],\n',
        '  "allowUnsafeCode": true,\n',
        '  "overrideReferences": false,\n',
        '  "precompiledReferences": [],\n',
        '  "autoReferenced": true,\n',
        '  "defineConstraints": [],\n',
        '  "versionDefines": [],\n',
        '  "noEngineReferences": false\n',
        '}\n'
    ])
    f.close()
    print("Created C# source asmdef file successfully!")


def main():
    input_args = get_arguments_commandline()
    generation_folder = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_destination_path, input_args.generation_folder)
    csharp_source_folder = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_csharp_source_path)
    teamcity_folder = os.path.dirname(os.path.realpath(__file__))
    create_output_path(generation_folder)
    package_dir_valid, windows_items, mac_items, android_items, ios_items, linux_items = copy_packages_in(input_args, generation_folder)

    if package_dir_valid == True:
        copy_wrapper_source(csharp_source_folder, generation_folder)
        create_wrapper_asmdef_file(input_args, os.path.join(generation_folder, 'Source'))
        create_package_file(input_args, generation_folder)
        create_initial_template(input_args, teamcity_folder, generation_folder)
        inject_metafile_data(input_args, generation_folder, teamcity_folder, windows_items, mac_items, android_items, ios_items, linux_items)
        generate_final_package(input_args, generation_folder)
        

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print("An error occurred, code (%s)." % e)
        exit(1)
    