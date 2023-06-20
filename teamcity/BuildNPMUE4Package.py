import os
import argparse
import shutil
import subprocess
import json

from yaml import load, dump

try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper


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
    parser.add_argument('--relative_linux_path',
                        help="Enter the relative path from root/teamcity for mac.",
                        default=None)
    parser.add_argument('--relative_ios_path',
                        help="Enter the relative path from root/teamcity for ios.",
                        default=None)
    parser.add_argument('--relative_include_path',
                        help="Enter the relative path from root/teamcity for the include directories.",
                        default=None)
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
                     
    args = parser.parse_args()

    return args


def copytree(src, dst, symlinks=False, ignore=None):
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, symlinks, ignore)
        else:
            shutil.copy2(s, d)


def create_output_path(output_path):
    try:
        if not os.path.exists(os.path.dirname(output_path)):
            os.mkdir(os.path.dirname(output_path))

        if not os.path.exists(output_path):
            os.mkdir(output_path)
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
        print("Created folder successfully!")
    except IOError as e:
        print("Issue creating folder, Error code (%s)." % e)
        raise FileHandlingError("Issue creating folder, Error code (%s)." % e)


def copy_packages_in(input_args, output_path):
    input_paths = []

    if(input_args.relative_windows_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_windows_path))

    if(input_args.relative_mac_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_mac_path))

    if(input_args.relative_android_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_android_path))

    if(input_args.relative_ios_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_ios_path))

    if(input_args.relative_linux_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_linux_path))
        
    if(input_args.relative_include_path):
        input_paths.append(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_include_path))

    for binary_folder in input_paths:
        copytree(binary_folder, os.path.join(output_path, os.path.basename(binary_folder)))
    
    # Copy in .npmrc file
    shutil.copy2(os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), "Library/Binaries/ue4_npmrc", ".npmrc"), output_path)

    if(input_paths):
        return True
    else:
        return False


def create_package_file(input_args, output_path):
    print("Creating package file...")
    f = open(os.path.join(output_path, "package.json"), "x")
    f.writelines([
        '{\n',
       f'  "name": "{input_args.name}",\n',
       f'  "displayName": "{input_args.display_name}",\n',
       f'  "version": "{input_args.version}",\n',
       f'  "description": "{input_args.description}",\n',
       f'  "license": "{input_args.license}",\n',
        '  "dependencies": {\n'
        '  },\n',
        '  "publishConfig": {\n',
       f'    "registry": "{input_args.registry}"\n',
        '  }\n'
        '}\n'
    ])
    f.close()
    print("Created package file successfully!")
    

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


def generate_final_package(input_args, generation_folder):
    print("Attempting to generate and publish npm package...")
    os.chdir(generation_folder)
    
    subprocess.call("docker run --rm -w /src -v " + generation_folder + ":/src node:lts-bullseye-slim npm install . --userconfig=.npmrc && npm " + input_args.release_mode, shell=True)
        
    print("Published npm package!")


def main():
    input_args = get_arguments_commandline()
    generation_folder = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_destination_path, input_args.generation_folder)
    create_output_path(generation_folder)
    package_dir_valid = copy_packages_in(input_args, generation_folder)

    if package_dir_valid == True:
        create_package_file(input_args, generation_folder)
        generate_final_package(input_args, generation_folder)
        

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print("An error occurred, code (%s)." % e)
        exit(1)
    