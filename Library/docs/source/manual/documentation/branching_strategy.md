# Branching Strategy

This document outlines the branching strategy as defined by the team.

## Branches

### [develop](https://github.com/magnopus-opensource/connected-spaces-platform/tree/develop)

The develop branch serves as the primary branch for ongoing development. It is locked to prevent direct commits. As such, any new features and fixes must go through the Pull Request (PR) approval process before landing in this branch. PRs at this level undergo build checks and tests, and require 2 review approvals to pass. PRs targeting `develop` should use the `Squash and merge` option in GitHub when the PR contains multiple commits to keep commit history clean. When doing so, please leave the commit description in its auto-generated form, as this helps generate correct changelog notes.

### [Main](https://github.com/magnopus-opensource/connected-spaces-platform/tree/Main)

The `Main` branch is used exclusively for hosting release versions of the Connected Spaces Platform library. It contains the code for the latest stable release, and tags are created against this branch to mark release builds. PR checks do not run on this branch.

### Development Branches

Akin to _Feature Branches_ in GitFlow, these cover any change needed to the codebase. They all follow the same conventions and standards. These branches are to be made as and when they are needed, though they must follow specific naming patterns.

If the work being done is based on a Jira ticket, the ticket ID should be included in the branch name. If there is no ticket for the work, you may use `NT-0` in place of the ticket ID.

> `<work type>/XX-YY_Title`
> * work type - A label denoting what kind of work this branch covers. Should be one of either `fix`, `feature`, `doc`, or `misc`. `misc` branches cover refactoring work, style changes/fixes, and other work that does not fit in the aforementioned labels.
> * XX-YY - Either a Jira ticket ID (such as `TIC-267`, or `TC-3012`), or `NT-0`.
> * Title - The title should summarise the work being done in the branch. It should not be overly long or too technical, but it should give the reader a general idea of the branch's purpose. Any spaces that would be in the branch title should be replaced with underscores (_).

Example branch names:

* `fix/NT-0_Token_refresh_expiry_failure_hotfix`
* `feature/TIC-456_Add_Geolocation_System`
* `feature/TIC-5398_Create_new_doxygen_pages`
* `misc/NT-0_Fix_broken_formatting_in_UserSystem`

## Branching Workflow

The workflow provides a structured approach to managing the development process.

Here is a high-level overview of how a change moves from development through to release.

### Development

Developers create branches off the `develop` branch for implementing changes. Once complete, they initiate a PR to merge this branch back into `develop`. While not currently enforced by rule, we advise and expect that branches have a singular focus.
This means that fixes should not appear in a feature branch, and should instead exist in a purpose-made branch and merged separately. The same expectation applies to documentation changes or refactors that are unrelated to a feature being developed. It is acceptable and expected to have tests included in branches where features or fixes require them.

*Note that it is possible to work with multiple branches locally if there is a non-merged fix required for an in-development feature.* 

### Release Preparation

When the `develop` branch reaches a stable state with the desired set of features and fixes, it is merged into `Main` via PR.

**PRs for merging the develop branch into the `Main` branch should be merged using a regular merge commit, NOT `Squash` or `Rebase`!**

After merging to `Main`, the `Main` branch will not be one commit ahead of `develop`. To resolve this, you must rebase `Main` onto `develop` via the command line using the following:
1. `git checkout Main`
2. `git pull`
3. `git checkout develop`
4. `git rebase Main`

### Release Deployment
New CSP packages are published via the `Publish CSP` deploy step on Team City. This will create a new tag for the release on the `Main` branch, create a new GitHub release and push build artifacts, and publish new `CSP for Unity` and `CSP for Web` NPM packages to npmjs. 

### Maintenance and Hotfixes

If any critical issues arise in a release version, a hotfix must be created as a fix branch and merged into the `develop` branch, then go through the standard release process, with a relevant version number increment.

## Examples

Below are some example scenarios illustrating how the process and workflow works in practice.

### Working on a new feature

1. Clone `develop` branch.
2. Create a new branch for your feature (Use the naming convention for branches as above).
3. Submit a PR to the `develop` branch once work is complete.
4. Await for PR build checks to validate the change is stable
5. Gain approval from two members of the Connected Spaces platform team
6. Merge to `develop`

### Making a new release version

1. Once `develop` has been validated as stable and complete, a PR is made to merge into `Main`.
2. Once merged into `Main`, a tag is applied to mark the version, and builds are made of the associated packages.
