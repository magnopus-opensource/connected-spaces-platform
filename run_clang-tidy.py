import glob
import os
import subprocess

# todo: make these command-line arguments?
VERBOSE = True
platform = "x64"
configuration = "DebugDLL"

extensions_to_scan = [".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx"] # case-insensitive search. add new formats in lower case.
locations_to_scan = ["MultiplayerTestRunner", "Tests", "Library"] #todo: others?
locations_to_exclude = ["thirdparty", "modules"] # case-insensitive search

# todo: ****BUILD*** the configuration using  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON (how???) so that the .\MultiplayerTestRunner\Intermediate\x64\DebugDLL\MultiplayerTestRunner.ClangTidy folders exist

# scan the files
for location in locations_to_scan:
    if (VERBOSE): print("Processing location " + location + "...")
    files_to_scan = []
    for file in glob.iglob(os.path.join(location, '**', '*'), recursive=True):
        if not any([x.lower() in file.lower() for x in locations_to_exclude]):
            base, extension = os.path.splitext(file)
            if extension.lower() in extensions_to_scan:
                files_to_scan.append(file)
    
    # run clang-tidy
    if (VERBOSE): print("Starting clang-tidy on " + location + "... Found " + str(len(files_to_scan)) + " files to process." )
    # clang-tidy.exe --config-file=.clang-tidy -p=".\MultiplayerTestRunner\Intermediate\x64\DebugDLL\MultiplayerTestRunner.ClangTidy" .\MultiplayerTestRunner\*
    command_to_run = ["clang-tidy.exe", "--config-file=.clang-tidy", "-p=.\\"+location+"\\Intermediate\\"+platform+"\\"+configuration+"\\"+location+".ClangTidy\\"] + files_to_scan
    if (VERBOSE): 
        print("Running command...")
        print(command_to_run)
        
    try:
        output = subprocess.check_output(command_to_run)
        if (VERBOSE): print("Command has finished running.")
        
        print(str(output))
        
    except subprocess.CalledProcessError as e:
        print(e.output)