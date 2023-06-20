#!python3

import chevron
import os
from git import Repo
import argparse
from datetime import datetime

#Converts text input of "True or False" to type <bool>
def str2bool(v):
    if isinstance(v, bool):
        return v
    if v.lower() in ('yes', 'true', 't', 'y', '1'):
        return True
    elif v.lower() in ('no', 'false', 'f', 'n', '0'):
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')
       
      
#Script CLI argument parser and help system
parser = argparse.ArgumentParser()

#Parser accepted arguments list
parser.add_argument("-ci", "--iscibuild", type=str2bool, nargs='?', const=True, default=False, help="Set foundation build source to signify it is a CI build eg --a or --auto True")
                        
args = parser.parse_args()

def init_git():
    return Repo(search_parent_directories=True) # Relative dir of the repo root

def main():
    git_repo = init_git()
    git_head_commit_id = git_repo.head.commit
    git_head_commit_date = git_head_commit_id.committed_date
    utc_time = datetime.utcfromtimestamp(git_head_commit_date)
    git_root = git_repo.git.rev_parse("--show-toplevel")
     
    foundation_build_id = utc_time.strftime("%y%m%d_%H%M%S") + "_" + str(git_head_commit_id)
    
    if (args.iscibuild == False):
        foundation_build_id = foundation_build_id + "/PERSONAL"
    print(foundation_build_id)
    
    output  = None


    with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'VersionTemplate.mustache'), 'r') as file_in:
        data = file_in.read()
        output = chevron.render(data, {'git_changelist_id': git_head_commit_id, 'foundation_build_id': foundation_build_id})
        
    with open(os.path.join(git_root, 'Library/include/CSP/version.h'), 'w') as f:
        f.write(output)

if __name__ == "__main__":
    main()
