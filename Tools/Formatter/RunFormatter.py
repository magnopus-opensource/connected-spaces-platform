import os
import stat
from glob import glob

# Get lists of all c/cpp/h/hpp files
types = ('*.c', '*.cpp', '*.h', '*.hpp') 
all_files = []
for file_type in types:
    all_files += [ y for x in os.walk(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))) for y in glob(os.path.join(x[0], file_type)) ]

# Refine all files by excluding unnecessary folders
all_files = [ x for x in all_files if "Tools" not in x ]
all_files = [ x for x in all_files if "ThirdParty" not in x ]
all_files = [ x for x in all_files if "packages" not in x ]
all_files = [ x for x in all_files if "teamcity" not in x ]
all_files = [ x for x in all_files if "modules" not in x ]
all_files = [ x for x in all_files if "codegen" not in x ]

# Run Clang Format on all files identified
for file_item in all_files:
    print("Applying Clang-Format to: " + file_item)
    os.chmod(file_item, stat.S_IWRITE) # Ensure the script can modify the file without error
    os.system("clang-format -i " + file_item)