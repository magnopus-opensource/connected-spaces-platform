import argparse
import os
import re
from git import Repo


def get_arguments_commandline():
    parser = argparse.ArgumentParser("Generate README")    
    parser.add_argument('--relative_output_path',
                        help="Enter the relative path from root/teamcity.", 
                        default="Library\\Binaries\\x64\\DebugDLL")
    parser.add_argument('--initial_commit',
                        help="Enter the initial commit to generate the README from. If no value is provided, logs will be generated from the commit of the latest tag.",
                        default='')
    parser.add_argument('--branch',
                        help="Enter the branch to generate the README within.",
                        default='origin/main')

    return parser.parse_args()


def init_git():
    repo = Repo(search_parent_directories=True) # Relative dir of the repo root

    return repo


def get_commits(input_args, repo):
    sha = repo.rev_parse(input_args.branch)
    diff_str = str(sha) + "..." + input_args.initial_commit
    commits = list(repo.iter_commits(diff_str))

    return commits

    
def get_jira_id(commit_message):
    if re.match("^(\[)+[a-z,A-Z]+[-]+[0-9]+(\])", commit_message):
        return commit_message.split('[')[1].split(']')[0]
    elif "Jobs:" in commit_message:
        ticket_id = commit_message.split('Jobs:')[1].split('\n')[0]

        if ' ' in ticket_id:
            ticket_id = ticket_id.split(' ')[1]

        ticket_id = ticket_id.split('-')[0] + '-' + ticket_id.split('-')[1]

        return ticket_id

    return None


def write_file_header(readme_path):
    file_out = open(readme_path, "w+")
    file_out.write("\n<h2>Revision Change Details</h2>\n\n<table><thead><tr><th>change_id</th><th>user</th><th>description</th></thead>\n<tbody>")

    return file_out


def write_file_line(change_id, change_user, change_desc, file_out):
    file_out.write('<tr><td>')
    file_out.write(change_id)
    file_out.write('</td><td>')
    file_out.write(change_user)
    file_out.write('</td><td>')
    file_out.write(change_desc)
    file_out.write('</td>')
    file_out.write('</tr>\n')

    
def write_file_footer(file_out):
    file_out.write('</tbody></table>\n')
    file_out.close()


def process_prefix(line_value, commit_id):
    commit_title = line_value.split(':', 1)
    breaking_change = False

    if len(commit_title) > 1:
        if '!' in commit_title[0]:
            line_value = "<p style=\"color:red\"><strong><em>" + commit_title[0].lower() + "</em></strong>:" + commit_title[1] + " </p> "
            breaking_change = True
        else:
            line_value = "<strong><em>" + commit_title[0].lower() + "</em></strong>:" + commit_title[1]

    return line_value, breaking_change


def process_commit_title(commit_title, commit_id):
    commit_title, breaking_change = process_prefix(commit_title, commit_id)

    if '#NoTicket' in commit_title:
        commit_title = commit_title.replace('#NoTicket', '<strong>#NoTicket</strong>')

    return commit_title.lower(), breaking_change


def process_commit_body(commit_id, commit_title, commit_body_list, breaking_change):
    result_body = " <em> "

    for commit_line in commit_body_list:
        if commit_line:
            commit_line = commit_line.replace("  ", " ") #Remove double space
            commit_line = " <br>" + commit_line
            result_body += commit_line + ' '

    return result_body + " </em> "


def process_change_description(change_desc, change_id):
    change_split = change_desc.split('\n')

    if len(change_split) > 1:
        commit_title, breaking_change = process_commit_title(change_split[0], change_id)
        change_split.pop(0) #Remove first element
        result_text = commit_title + process_commit_body(change_id, cleanhtml(commit_title), change_split, breaking_change)
    else:
        commit_title = ''
        result_text, breaking_change = process_commit_title(change_desc, change_id)

    return result_text, commit_title


def cleanhtml(raw_html):
  cleanr = re.compile('<.*?>')
  cleantext = re.sub(cleanr, '', raw_html)

  return cleantext


def create_summary_list(commit_title, commit_id, jira_job_id, feature_commit_list, fix_commit_list, style_commit_list, refactor_commit_list, test_commit_list, doc_commit_list, breaking_commit_list, misc_commit_list):
    raw_commit_title = cleanhtml(commit_title)

    commit_list = raw_commit_title.split(': ')

    if len(commit_list) > 1:
        commit_tag = commit_list[0]
        commit_description = commit_list[1]

        if "feat" in commit_tag:
            if jira_job_id != None:
                feature_commit_list.append("[" + jira_job_id + "]" + " - "  + commit_description.capitalize() + " - " + commit_id )
            else:
                feature_commit_list.append("No Ticket - " + commit_description.capitalize() + " - " + commit_id )

        elif "fix" in commit_tag:
            if jira_job_id != None:
                fix_commit_list.append("[" + jira_job_id + "]" + " - "  + commit_description.capitalize() + " - " + commit_id )
            else:
                fix_commit_list.append("No Ticket - " + commit_description.capitalize() + " - " + commit_id )

        elif "style" in commit_tag:
            if jira_job_id != None:
                style_commit_list.append("[" + jira_job_id + "]" + " - "  + commit_description.capitalize() + " - " + commit_id )
            else:
                style_commit_list.append("No Ticket - " + commit_description.capitalize() + " - " + commit_id )

        elif "test" in commit_tag:
            if jira_job_id != None:
                test_commit_list.append("[" + jira_job_id + "]" + " - "  + commit_description.capitalize() + " - " + commit_id )
            else:
                test_commit_list.append("No Ticket - " + commit_description.capitalize() + " - " + commit_id )

        elif "doc" in commit_tag:
            if jira_job_id != None:
                doc_commit_list.append("[" + jira_job_id + "]" + " - "  + commit_description.capitalize() + " - " + commit_id )
            else:
                doc_commit_list.append("No Ticket - " + commit_description.capitalize() + " - " + commit_id )

        elif "refac" in commit_tag:
            if jira_job_id != None:
                refactor_commit_list.append("[" + jira_job_id + "]" + " - "  + commit_description.capitalize() + " - " + commit_id )
            else:
                refactor_commit_list.append("No Ticket - " + commit_description.capitalize() + " - " + commit_id )

        elif "!" in commit_tag:
            if jira_job_id != None:
                breaking_commit_list.append("[" + jira_job_id + "]" + " - "  + commit_description.capitalize() + " - " + commit_id )
            else:
                breaking_commit_list.append("No Ticket - " + commit_description.capitalize() + " - " + commit_id )
        
        # Case to cover situations where the commit has been modified in a bad format that still contains a tag but not a recognised one
        else:
            misc_commit_list.append("No Ticket - " + commit_description.capitalize() + " - " + commit_id )
    # Case to cover situations where the commit has been modified to not comply to our standard format, with the tag. We want to avoid printing merge commits from internal branches, however.
    else:
        if 'magnopus-opensource/develop' not in raw_commit_title and 'magnopus-opensource/main' not in raw_commit_title and 'magnopus-opensource/staging' not in raw_commit_title:
            misc_commit_list.append("No Ticket - " + raw_commit_title.capitalize() + " - " + commit_id )


def create_summaries_text(input_args, feature_commit_list, fix_commit_list, style_commit_list, refactor_commit_list, test_commit_list, doc_commit_list, breaking_commit_list, misc_commit_list):
    output_text = "<h1>Built from changelist ID " + input_args.initial_commit + "</h1>\n<h2>Summary Lists</h2>\n"

    if feature_commit_list:
        output_text += "<h3><p>&#x1F370; &#x1F64C; New Features</p></h3>\n<ul>"

        for commit in feature_commit_list:
            output_text += "<li>" + commit + "</li>"

        output_text += "</ul>"

    if fix_commit_list:
        output_text += "<h3><p>&#x1F41B; &#x1F528; Fixes</p></h3>\n<ul>"

        for commit in fix_commit_list:
            output_text += "<li>" + commit + "</li>"

        output_text += "</ul>"

    if style_commit_list:
        output_text += "<h3><p>&#x1F60E; Style Changes</p></h3>\n<ul>"

        for commit in style_commit_list:
            output_text += "<li>" + commit + "</li>"

        output_text += "</ul>"

    if refactor_commit_list:
        output_text += "<h3><p>&#x1F4AB; &#x1F4A5; Refactor</p></h3>\n<ul>"

        for commit in refactor_commit_list:
            output_text += "<li>" + commit + "</li>"

        output_text += "</ul>"

    if test_commit_list:
        output_text += "<h3><p>&#x1F648; &#x1F649; &#x1F64A; Test Changes</p></h3>\n<ul>"

        for commit in test_commit_list:
            output_text += "<li>" + commit + "</li>"

        output_text += "</ul>"

    if doc_commit_list:
        output_text += "<h3><p>&#x1F4D6; &#x270D; Document Changes</p></h3>\n<ul>"

        for commit in doc_commit_list:
            output_text += "<li>" + commit + "</li>"

        output_text += "</ul>"

    if breaking_commit_list:
        output_text += "<h3><p>&#x1F525; &#x2757; Breaking Changes</p></h3>\n<ul>"

        for commit in breaking_commit_list:
            output_text += "<li>" + commit + "</li>"

        output_text += "</ul>"

    if misc_commit_list:
        output_text += "<h3><p>&#x1F9E9; Misc Changes</p></h3>\n<ul>"

        for commit in misc_commit_list:
            output_text += "<li>" + commit + "</li>"

        output_text += "</ul>"
    return output_text


def write_prepend_to_output_file(readme_path, prepend_summaries):
    read_obj = open(readme_path, 'r')
    read_lines = read_obj.readlines()
    read_obj.close()

    with open(readme_path, 'w') as write_obj:
        write_obj.write(prepend_summaries)

        for line in read_lines:
            write_obj.write(line)


def main():
    input_args = get_arguments_commandline()
    feature_commit_list = []
    fix_commit_list = []
    style_commit_list = []
    refactor_commit_list = []
    test_commit_list = []
    doc_commit_list = []
    breaking_commit_list = []
    misc_commit_list = []

    git_repo = init_git()

    if git_repo:
        if input_args.initial_commit == '':
            # If there are no tags we go back to the first commit on the branch
            if len(git_repo.tags) > 0:
                # Get commit ID of latest tag
                latest_tag = git_repo.git.describe(tags=True, abbrev=0)
                tag_commit_id = git_repo.git.rev_list(latest_tag, n=1)
                input_args.initial_commit = tag_commit_id
            else:
                first_commit = list(git_repo.iter_commits())[-1]
                input_args.initial_commit = first_commit.hexsha

        commit_list = get_commits(input_args, git_repo)

        if commit_list:
            print("Collected " + str(len(commit_list)) + " changes, now processing into release notes")

            if len(commit_list) == 0:
                print("No changes to summarise!")
                return

            readme_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", input_args.relative_output_path, "README.md")
            file_out = write_file_header(readme_path)

            for commit_item in commit_list:
                author = commit_item.author
                hexsha = commit_item.hexsha
                hexsha = hexsha[0:7]
                message = commit_item.message

                if 'Build' != author.name:
                    change_desc, commit_title = process_change_description(message, hexsha)
                    
                    jira_job_id = get_jira_id(message)
                    
                    # Only include merge commits as these are the commits that are the results of a PR. We do further screening inside the create_summary_list method to ensure we only print the merges we want to.
                    if (cleanhtml(commit_title).startswith("merge pull request")):
                        create_summary_list(change_desc, hexsha, jira_job_id, feature_commit_list, fix_commit_list, style_commit_list, refactor_commit_list, test_commit_list, doc_commit_list, breaking_commit_list, misc_commit_list)
                    write_file_line(hexsha, author.name, change_desc, file_out)

            write_file_footer(file_out)
            file_out.close()
            prepend_summaries = create_summaries_text(input_args, feature_commit_list, fix_commit_list, style_commit_list, refactor_commit_list, test_commit_list, doc_commit_list, breaking_commit_list, misc_commit_list)
            write_prepend_to_output_file(readme_path, prepend_summaries)
            print("Finished processing file, now exiting process")
        else:
            print("No readme generated as no changes found!")


if __name__ == "__main__":
    main()
