import glob
import os
import shutil
import subprocess
import sys

# todo: make these command-line arguments?
VERBOSE = True
PLATFORM = "x64"
CONFIGURATION = "DebugDLL"
# todo: get path to MSBuild.exe from env var? os.getenv('MY_SUPER_DUPER_ENV_VAR')
MSBUILD_EXE = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe"

EXTENSIONS_TO_SCAN = [".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx"] # case-insensitive search. add new formats in lower case.
LOCATIONS_TO_SCAN = ["Library", "MultiplayerTestRunner", "Tests"]
LOCATIONS_TO_EXCLUDE = ["ThirdParty", "modules"] # case-insensitive search


for location in LOCATIONS_TO_SCAN:

    if VERBOSE: print("Processing location " + location + "...")

    if (location == "Library"):
        clang_tidy_name = "ConnectedSpacesPlatform_D"
        target_name = ".\\ConnectedSpacesPlatform.sln"
    else:
        clang_tidy_name = location
        target_name = ".\\" + location

    # generate the compile_commands.json file in e.g. .\MultiplayerTestRunner\Intermediate\x64\DebugDLL\MultiplayerTestRunner.ClangTidy
    compile_file_path = os.path.join(location, "Intermediate", PLATFORM, CONFIGURATION, clang_tidy_name + ".ClangTidy")
    if os.path.exists(compile_file_path):
        shutil.rmtree(compile_file_path)
        if VERBOSE: print("compile_commands.json was successfully removed from " + compile_file_path)

    # This command fails to actually build the project, but we don't care as we just want to generate compile_commands.json
    # We run clang-tidy later on with a lot more degrees of freedom (like excludng files) than what would be possible with this.
    subprocess.run([MSBUILD_EXE,
                    "/t:ClangTidy",
                    "/p:Configuration=" + CONFIGURATION,
                    "/p:Platform=" + PLATFORM,
                    target_name])
    if os.path.exists(compile_file_path):
        if VERBOSE: print("compile_commands.json was successfully generated for " + location + ".")
    else:
        print("Error: compile_commands.json was NOT generated for " + location + ". Aborting...")
        sys.exit(-1)

    # scan the files
    files_to_scan = []
    for file in glob.iglob(os.path.join(location, '**', '*'), recursive=True):
        if not any([x.lower() in file.lower() for x in LOCATIONS_TO_EXCLUDE]):
            base, extension = os.path.splitext(file)
            if extension.lower() in EXTENSIONS_TO_SCAN:
                files_to_scan.append(file)

    # run clang-tidy
    if VERBOSE: print("Starting clang-tidy on " + location + "... Found " + str(len(files_to_scan)) + " files to process." )

    # clang-tidy.exe --config-file=.clang-tidy -p=".\MultiplayerTestRunner\Intermediate\x64\DebugDLL\MultiplayerTestRunner.ClangTidy" < list of files in MultiplayerTestRunner >
    command_to_run = ["clang-tidy.exe", "--config-file=.clang-tidy", "-p=.\\" + location + "\\Intermediate\\" + PLATFORM + "\\" + CONFIGURATION + "\\" + clang_tidy_name + ".ClangTidy\\"] + files_to_scan
    if VERBOSE:
        print("Running clang-tidy command on " + location + "...")
        print(command_to_run)

    try:
        subprocess.run(command_to_run)
        if VERBOSE: print("Clang-tidy command has finished running on " + location + ".")

        # todo! we have to process the output in order to retrieve all the errors and warnings that clang-tidy found
        # output = subprocess.check_output(command_to_run)
        # print(str(output)) # this is completely unreadable

    except subprocess.CalledProcessError as e:
        print(e.output)
