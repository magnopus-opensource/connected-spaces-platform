import os
import glob

def fix_COMSPEC_variable_casing():
    current_dir = os.getcwd()
    print("Fixing the COMSPEC casing recursively in ", current_dir)
    list_of_makefiles = glob.glob(current_dir + "/**/Makefile", recursive = True)

    counter = 0
    changed_files=[]
    for makefile in list_of_makefiles:
        try_to_change = False
        with open(makefile,'r') as file:
            content = file.read()
            newContent = content.replace('$(ComSpec)', '$(COMSPEC)')
            if newContent != content:
                try_to_change = True    

        try:
            if try_to_change == True:
                with open(makefile,'w') as file:
                    file.write(newContent)
                    counter = counter + 1
                    changed_files.append(makefile)    
        except:
            print("File ", makefile, "cannot be written")

    print("\n")
    print("Changed ", counter, "files")
    print(changed_files)
    
fix_COMSPEC_variable_casing()