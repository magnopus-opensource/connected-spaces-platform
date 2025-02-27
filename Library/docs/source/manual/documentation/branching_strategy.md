# Branching Strategy

The CSP team are in the process of adopting the Trunk-Based Development (TBD) version control strategy. As such we have a single `main` branch of the codebase against which all work must be completed. The `main` branch must always be in a stable state ready to be deployed.

## Branches

### [main](https://github.com/magnopus-opensource/connected-spaces-platform/tree/main)

The `main` branch is locked to prevent direct commits. As such, any new features and fixes must go through the Pull Request (PR) approval process before landing in this branch. PRs undergo build checks and tests, and require 2 review approvals to pass. PRs targeting `main` should use the `Squash and merge` option in GitHub when the PR contains multiple commits to keep commit history clean. When doing so, please leave the commit description in its auto-generated form, as this helps generate correct changelog notes.

CSP releases are made from the `main` branch, and tags are created against this branch to mark release builds.

### Development Branches

Akin to _Feature Branches_ in GitFlow, these cover any change needed to the codebase. They all follow the same conventions and standards. These branches are to be made as and when they are needed, though they must follow specific naming patterns.

If the work being done is based on a Jira ticket, the ticket ID should be included in the branch name. If there is no ticket for the work, you may use `NT-0` in place of the ticket ID.

> `<work type>/XX-YY_Title`
> * work type - A label denoting the commit category. We have adopted the Conventional Commits specification, and so this label should be one of the following: `fix`, `feat`, `build`, `chore`, `ci`, `docs`, `style`, `refac`, `perf` or `test`.
> * XX-YY - Either a Jira ticket ID (such as `TIC-267`, or `TC-3012`), or `NT-0`.
> * Title - The title should summarise the work being done in the branch. It should not be overly long or too technical, but it should give the reader a general idea of the branch's purpose. Any spaces that would be in the branch title should be replaced with underscores (_).

Example branch names:

* `fix/NT-0_Token_refresh_expiry_failure_hotfix`
* `feature/TIC-456_Add_Geolocation_System`
* `feature/TIC-5398_Create_new_doxygen_pages`
* `refac/NT-0_Refac_formatting_in_UserSystem`

## Branching Workflow

The TBD workflow provides a structured approach to managing the development process.

Here is a high-level overview of how a change moves from development through to release.

### Development

Developers create branches off the `main` branch for implementing changes. Once complete, they initiate a PR to merge this branch back into `main`. While not currently enforced by rule, we advise and expect that branches have a singular focus.
This means that fixes should not appear in a feature branch, and should instead exist in a purpose-made branch and merged separately. The same expectation applies to documentation changes or refactors that are unrelated to a feature being developed. It is acceptable and expected to have tests included in branches where features or fixes require them.

*Note that it is possible to work with multiple branches locally if there is a non-merged fix required for an in-development feature.* 

### Release Deployment
New CSP packages are published via the `Publish CSP` deploy step on Team City. This will create a new tag for the release on the `main` branch, create a new GitHub release and push build artifacts, and publish new `CSP for Unity` and `CSP for Web` NPM packages to npmjs. 

### Maintenance and Hotfixes

If any critical issues arise in a release version, a hotfix must be created as a fix branch and merged into the `main` branch, then go through the standard release process, with a relevant version number increment.

## Examples

Below are some example scenarios illustrating how the process and workflow works in practice.

### Working on a new feature

1. Clone `main` branch.
2. Create a new branch for your feature (Use the naming convention for branches as above).
3. Submit a PR to the `main` branch once work is complete.
4. Await for PR build checks to validate the change is stable
5. Gain approval from two members of the Connected Spaces platform team
6. Merge to `main`
7. Once merged into `main`, a tag is applied to mark the version, and builds are made of the associated packages.
