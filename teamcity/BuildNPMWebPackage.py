import os
import argparse
import shutil
import subprocess

from Config import config

class ConnectedSpacesPlatformPyError(Exception): pass
class FileHandlingError(ConnectedSpacesPlatformPyError): pass


def get_arguments_commandline():
    parser = argparse.ArgumentParser(description='Build the Connected Spaces Platform Release for NPM.')

    parser.add_argument('--version',
                        help="Enter the version of the Connected Spaces Platform, Semantic Versioning Only.",
                        default="0.0.0")
    parser.add_argument('--name',
                        help="Enter the name of the Connected Spaces Platform package.",
                        default="connected-spaces-platform.web")
    parser.add_argument('--display_name',
                        help="Enter the display name of the Connected Spaces Platform package.",
                        default="connected-spaces-platform.web")
    parser.add_argument('--relative_destination_path',
                        help="Enter the relative path from root/teamcity for the libraries to be copied to.",
                        default="Library\\Binaries\\package\\wasm")
    parser.add_argument('--relative_wasm_path',
                        help="Enter the relative path from root/teamcity for wasm.",
                        default="Library\\Binaries\\wasm")
    parser.add_argument('--relative_typescript_path',
                        help="Enter the relative path from root/teamcity for typescript.",
                        default="Tools\\WrapperGenerator\\Output\\TypeScript")
    parser.add_argument('--license',
                        help="Enter the license required for the Connected Spaces Platform package.",
                        default="Apache-2.0")
    parser.add_argument('--dependencies',
                        help="Enter the dependencies required for the Connected Spaces Platform package.",
                        default=None)
    parser.add_argument('--description',
                        help="Enter the description for the Connected Spaces Platform package.",
                        default="This package provides the binaries required to interface with the Connected Spaces Platform API.")
    parser.add_argument('--registry',
                        help="This is the upstream location of the package.",
                        default="https://npm.pkg.github.io/@magnopus-opensource")
    parser.add_argument('--generation_folder',
                        help="This is the package generation location.",
                        default="package_gen")
    parser.add_argument('--release_mode',
                        help="NPM release command, pack and publish are available.",
                        default="pack")
    parser.add_argument('--scope',
                        help="Enter the scope of the published package. Appends the registry URL.",
                        default=None)
    parser.add_argument('--npm_publish_flag',
                        help="Whether the package should be published to NPM. Default == True",
                        default=True)
                     
    args = parser.parse_args()

    return args


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
    print("Copying packages...")
    rel_wasm_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_wasm_path)
    if os.path.isdir(rel_wasm_path):
        input_paths.append(rel_wasm_path)
        print("Wasm path created")

    rel_typescript_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_typescript_path)
    if os.path.isdir(rel_typescript_path):
        input_paths.append(rel_typescript_path)
        print("typescript path created")

    for path in input_paths:
        for file in os.listdir(path):
            file_path = os.path.join(path, file)

            if os.path.isdir(file_path):
                shutil.copytree(file_path, output_path + "\\" + file, dirs_exist_ok=True)
            else:
                shutil.copy2(file_path, output_path)

    if(input_paths):
        print("input paths true")
        return True
    else:
        print("input paths false")
        return False

def copy_readme(input_args, output_path):
    # Copy readme
    if (os.path.exists(f"{config.default_output_directory}/README.md")):
        shutil.copy(f"{config.default_output_directory}/README.md", output_path)

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
        '  "main": "./connectedspacesplatform.js",\n',
        '  "types": "./connectedspacesplatform.d.ts",\n',
        '  "publishConfig": {\n',
       f'    "registry": "{input_args.registry}/{input_args.scope}"\n',
        '  },\n'
        '  "repository": "https://github.com/magnopus-opensource/connected-spaces-platform"'
        '}\n'
    ])
    f.close()
    print("Created package file successfully!")


def generate_final_package(input_args, generation_folder):
    print("Attempting to generate and publish npm package...")
    os.chdir(generation_folder)
    
    subprocess.call("docker run --rm -w /src -v " + generation_folder + ":/src node:lts-bullseye-slim npm install . --userconfig=.npmrc && npm " + input_args.release_mode + f" --\"@magnopus-opensource:registry={input_args.registry}\"", shell=True)
        
    print("Published npm package!")


def main():
    input_args = get_arguments_commandline()
    generation_folder = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), input_args.relative_destination_path, input_args.generation_folder)
    create_output_path(generation_folder)
    package_dir_valid = copy_packages_in(input_args, generation_folder)

    if package_dir_valid == True:
        copy_readme(input_args, generation_folder)
        create_package_file(input_args, generation_folder)
        if input_args.npm_publish_flag == True:
            generate_final_package(input_args, generation_folder)
        

if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print("An error occurred, code (%s)." % e)
        exit(1)
    