import glob
import os
import shutil
import subprocess
import sys

# todo: make these command-line arguments?
VERBOSE = True
PLATFORM = "x64"
CONFIGURATION = "DebugDLL"

# We're using Visual Studio Professional 2022 on both the CI and locally, so at this point there is no need to read this path dynamically.
MSBUILD_EXE = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe"

EXTENSIONS_TO_SCAN = [".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx"] # case-insensitive search. add new formats in lower case.
LOCATIONS_TO_SCAN = ["Library", "MultiplayerTestRunner", "Tests"]
LOCATIONS_TO_EXCLUDE = ["ThirdParty", "modules"] # case-insensitive search

errors_in_location = [False] * len (LOCATIONS_TO_SCAN)

for index, location in enumerate(LOCATIONS_TO_SCAN):

    if VERBOSE: print("Processing location " + location + "...")

    if (location == "Library"):
        clang_tidy_name = "ConnectedSpacesPlatform"
        if "Debug" in CONFIGURATION:
            clang_tidy_name += "_D"
        target_name = ".\\ConnectedSpacesPlatform.sln"
    else:
        clang_tidy_name = location
        target_name = ".\\" + location

    # generate the compile_commands.json file in e.g. .\MultiplayerTestRunner\Intermediate\x64\DebugDLL\MultiplayerTestRunner.ClangTidy
    compile_file_path = os.path.join(location, "Intermediate", PLATFORM, CONFIGURATION, clang_tidy_name + ".ClangTidy")
    if os.path.exists(compile_file_path):
        shutil.rmtree(compile_file_path)
        if VERBOSE: print("compile_commands.json was successfully removed from " + compile_file_path)

    # This command fails to actually build the project on the x64 platform, but we don't care as we just want to generate compile_commands.json. So we're happy that it fails quickly.
    # We run clang-tidy later on with a lot more degrees of freedom (like excluding files) than what would be possible with this.
    # Note if we ever want to run this on the Android platform: the command below "just works" and does not create an intermediary compile_commands.json file.
    # However, it scans all the files, including modules and third parties, which we can't exclude this way.
    command_to_run = [MSBUILD_EXE,
                    "/t:ClangTidy",
                    "/p:Configuration=" + CONFIGURATION,
                    "/p:Platform=" + PLATFORM,
                    target_name]
    if VERBOSE:
        print("Generating compile_commands.json file with command...")
        print(command_to_run)
    subprocess.run(command_to_run)
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
            print("\tThere were errors in location " + LOCATIONS_TO_SCAN[index] + ".")
    sys.exit(-1)

# That's all, folks!
sys.exit(os.EX_OK)
