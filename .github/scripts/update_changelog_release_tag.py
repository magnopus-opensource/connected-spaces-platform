import sys
import os

def update_changelog(version, date, changelog_file_path):
    """
    Updates the CHANGELOG.md file by replacing:
    '## [Unreleased]'
    with:
    '## [Unreleased]\n\n\n## [VERSION] - DATE'.
    This ensures that a new '## [Unreleased]' tag is ready for the next release.
    """
    print(f"Attempting to update {changelog_file_path} for release {version} on {date}")

    if not os.path.exists(changelog_file_path):
        print(f"Error: Changelog file not found at {changelog_file_path}")
        sys.exit(1)

    try:
        with open(changelog_file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()

        unreleased_found = False
        version_entry_found = False
        new_lines = []

        # Used below when checking for an existing version entry
        version_prefix = f"## [{version}] -"

        for line in lines:
            if line.strip() == "## [Unreleased]":
                unreleased_found = True
                # Keep the "## [Unreleased]" tag line and then add newlines and the new version entry below
                new_lines.append(line)
                new_lines.append(f"\n\n\n## [{version}] - {date}\n")
            # Check if the version entry already exists though ignore the date portion
            elif line.strip().startswith(version_prefix) and " - " in line.strip():
                version_entry_found = True
                new_lines.append(line)
            else:
                new_lines.append(line)

        if not unreleased_found:
            print("Error: '## [Unreleased]' section not found in CHANGELOG.md. No update performed.")
            # Set the 'changelog_updated' output variable for the invoking GitHub Action
            print("changelog_updated=false")
            sys.exit(1)

        if version_entry_found:
            print(f"Changelog already contains entry for {version}. No update needed.")
            # Exit successfully if no changes are needed.
            # Set the 'changelog_updated' output variable for the invoking GitHub Action
            print("changelog_updated=false")
            sys.exit(0)

        with open(changelog_file_path, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)

        print(f"Successfully updated CHANGELOG.md for release {version}.")
        # Set the 'changelog_updated' output variable for the invoking GitHub Action
        print("changelog_updated=true")
        sys.exit(0)

    except Exception as e:
        print(f"An error occurred while updating the changelog: {e}")
        # Set the 'changelog_updated' output variable for the invoking GitHub Action
        print("changelog_updated=false")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python update_changelog_release_tag.py <version> <date> <changelog_file_path>")
        sys.exit(1)

    release_version = sys.argv[1]
    release_date = sys.argv[2]
    changelog_path = sys.argv[3]

    update_changelog(release_version, release_date, changelog_path)