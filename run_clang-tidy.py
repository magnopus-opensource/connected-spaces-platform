import glob
import os
import shutil
import subprocess
import sys

# todo: make these command-line arguments?
VERBOSE = True
USE_MSBUILD_CLANGTIDY = True
USE_CLANGPLUSPLUS = not USE_MSBUILD_CLANGTIDY

PLATFORM = "x64"
CONFIGURATION = "DebugDLL"
COMPILE_FILE_NAME = "compile_commands.json"

TEMP_O = "temp.o"
TEMP_O_JSON = TEMP_O + ".json"


# We're using Visual Studio Professional 2022 on both the CI and locally, so at this point there is no need to read this path dynamically.
MSBUILD_EXE = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe"

EXTENSIONS_TO_SCAN = [".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx"] # case-insensitive search. add new formats in lower case.
LOCATIONS_TO_SCAN = ["Library", "MultiplayerTestRunner", "Tests"]
LOCATIONS_TO_EXCLUDE = ["ThirdParty", "modules"] # case-insensitive search

errors_in_location = [False] * len (LOCATIONS_TO_SCAN)

for index, location in enumerate(LOCATIONS_TO_SCAN):

    if VERBOSE: print("Processing " + location + "...")

    if (location == "Library"):
        clang_tidy_name = "ConnectedSpacesPlatform"
        if "Debug" in CONFIGURATION:
            clang_tidy_name += "_D"
        target_name = ".\\ConnectedSpacesPlatform.sln"
    else:
        clang_tidy_name = location
        target_name = ".\\" + location

    # Make sure there are no leftover compile files
    compile_file_path = os.path.join(location, "Intermediate", PLATFORM, CONFIGURATION, clang_tidy_name + ".ClangTidy")
    if os.path.exists(compile_file_path):
        shutil.rmtree(compile_file_path)
        if VERBOSE: print("compile_commands.json was successfully removed from " + compile_file_path)
    if os.path.exists(COMPILE_FILE_NAME):
        os.remove(COMPILE_FILE_NAME)
        if VERBOSE: print("compile_commands.json was successfully removed from .")

    if USE_MSBUILD_CLANGTIDY:
        # generate the compile_commands.json file in e.g. .\MultiplayerTestRunner\Intermediate\x64\DebugDLL\MultiplayerTestRunner.ClangTidy
        # This command SOMETIMES fails to actually build the project, but we don't care as we just want to generate compile_commands.json.
        # We (re-)run clang-tidy later on with a lot more degrees of freedom (like excluding files) than what would be possible with this.
        command_to_run = [MSBUILD_EXE,
                        "/t:ClangTidy",
                        "/p:Configuration=" + CONFIGURATION,
                        "/p:Platform=" + PLATFORM,
                        target_name]
        if VERBOSE:
            print("Generating compile_commands.json file with command...")
            print(command_to_run)
        try:
            output = subprocess.check_output(command_to_run)
            print(output)
        except subprocess.CalledProcessError as e:
            for line in str(e.output).split("\\n"):
                print(line)
            if VERBOSE: print("Compile command has finished running on " + location + " with at least one error.")

        if os.path.exists(compile_file_path):
            if VERBOSE: print("compile_commands.json was successfully generated for " + location + ".")
        else:
            print("Error: compile_commands.json was NOT generated for " + location + ". Aborting...")
            sys.exit(-1)


    # scan the files and generate compile_commands.json
    files_to_scan = []


    if USE_CLANGPLUSPLUS:
        compile_file = open(COMPILE_FILE_NAME,"w")

        file_counter = 0

    for file in glob.iglob(os.path.join(location, '**', '*'), recursive=True):
        if not any([x.lower() in file.lower() for x in LOCATIONS_TO_EXCLUDE]):
            base, extension = os.path.splitext(file)
            if extension.lower() in EXTENSIONS_TO_SCAN:
                files_to_scan.append(file)

                if USE_CLANGPLUSPLUS:
                # create the intermediary temp.o.json
                    file_counter += 1
                    if VERBOSE: print("Processing compile commands for file number " + str(file_counter) + "...")
                    subprocess.run(["clang++", "-MJ", TEMP_O_JSON, "-Wall",  "-o", TEMP_O, "-c", file, "-ferror-limit=1000"])
                    # the last argument is so that it does not stop when it reaches too many errors
                    # append it to the compile_commands.json file
                    with open("temp.o.json") as temp_file:
                        compile_file.write("[")
                        for line in temp_file.readlines():
                            compile_file.write(line)
                        compile_file.write("]")

    if USE_CLANGPLUSPLUS:
        # todo: remove the last comma in compile_commands.json??? seems ok without
        compile_file.close()

        # clean up
        if os.path.exists(TEMP_O_JSON): os.remove(TEMP_O_JSON)
        if os.path.exists(TEMP_O): os.remove(TEMP_O)

        if VERBOSE: print("compile_commands.json was successfully created.")

    # run clang-tidy
    if VERBOSE: print("Starting clang-tidy on " + location + "... Found " + str(len(files_to_scan)) + " files to process." )

    compile_file_location = ""
    if USE_CLANGPLUSPLUS:
        compile_file_location = "."
    if USE_MSBUILD_CLANGTIDY:
        compile_file_location = ".\\" + location + "\\Intermediate\\" + PLATFORM + "\\" + CONFIGURATION + "\\" + clang_tidy_name + ".ClangTidy\\"

    command_to_run = ["clang-tidy.exe", "--config-file=.clang-tidy", "-p=" + compile_file_location] + files_to_scan
    if VERBOSE:
        print("Running clang-tidy command on " + location + "...")
        print(command_to_run)

    try:
        output = subprocess.check_output(command_to_run)
        print(output) # todo: check what is in there once we have a run without errors (when?...)
        if VERBOSE: print("Clang-tidy command has finished running on " + location + ".")

    except subprocess.CalledProcessError as e:
        errors_in_location[index] = True
        for line in str(e.output).split("\\n"):
            print(line)
        if VERBOSE: print("Clang-tidy command has finished running on " + location + " with at least one error.")

if any(errors_in_location):
    print("\nSummary of errors:")
    for index, errlocation in enumerate(errors_in_location):
        if errlocation:
            print("\tThere were errors in " + LOCATIONS_TO_SCAN[index] + ".")
    sys.exit(-1)

# That's all, folks!
sys.exit(os.EX_OK)
