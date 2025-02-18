# Pull Request Guidelines

When making, or engaging with, a new pull request against main, please attempt to follow these guidelines.

> [!NOTE]
> All of these policies are general guidelines, and we acknowledge that exceptions to every rule exist. Use your best judgement, always.


### Comment Ladder
When leaving review comments, mark your comment with a tag demonstrating what the expected reaction to your comment is, in order to avoid text-comms based ambiguity.

![image](https://github.com/user-attachments/assets/9fd015c4-815a-4bd0-82b1-e27a6ab99717)


| Tag  | Is a response required? | Is a change required?  | May the PR author resolve |
| ------------- | ------------- | ------------- | ------------- |
| [Fix]  | ✔️  | ✔️ | ❌  |
| [Consider]  |✔️  | ❌  | ✔️  |
| [Question]  | ✔️  | ❌  | ❌  |
| [Nit]  | ❌  | ❌  | ✔️  |
| [Comment]  | ❌  | ❌  | ✔️  |

- **[Fix]** comments must be addressed, either by a change, or by the author convincing the reviewer that the change is not necessary. Once the change is made, the reviewer should evaluate the change, and resolve the conversation if it is acceptable.
- **[Consider]** comments are meaningful suggestions. They may be ignored, but the author should respond with justification as to why. This justification need not be accepted by the commenter, the reviewer is free to resolve the conversation.
- **[Question]** mandates a response, and the reviewer should either follow up, or resolve the conversation with a satisfactory answer.
- **[Nit]** is for nitpicks, stylistic or otherwise. Authors may choose to address these, or close the conversation with no action.
- **[Comment]** is similar to a nit, but does not even imply a desired change. These will almost always just be resolved by the PR author.

### Clean Commit History
Keep a clean, noise-free commit history when making a pull request. The following in particular are highly frowned upon when appearing in a changeset.
- Merge commits
- "Undo" commits. (Ie, commits that undo changes made earlier in the changeset)
- Changes that are already in main

Remember, you are walking the reviewer through the changeset, try to tell them a story. The fact that you changed your mind and undid a lot of work is not important, only the final result is.

> [!TIP]
> A simple, albeit crude way of curating your history is to perform `git reset SHA_OF_INITIAL_BRANCH_COMMIT --soft`, which will reset your branch to before you made any changes, whilst keeping all your changes locally.
> You may then stage them in a way that best tells the story of the change. Remember to take a backup/stash first, just to play it safe.

### Link Commits to Conversations
After having made a change in response to a suggestion by a reviewer, this change should be linked to the relevant conversation, so that the reviewer may evaluate the change.
This prevents reviewers from having to search through large changesets in order to validate whether or not particular comments have been addressed.

![image](https://github.com/user-attachments/assets/749326a2-7f21-4836-b635-4de2dd36898f)

> [!WARNING]  
> Once people have started reviewing a PR, the author should try to avoid force-pushing, as this breaks conversation links.
> Be sure to do all your history rewriting before marking the PR as ready to review.

### Merge via Squash
To keep things simple, all PRs are merged into main via squash. GitHub should only give you this option, so no need to think about it!

### Reviewer Assignment
When making a pull request, GitHub should automatically assign two reviewers from the connected-spaces-platform team.
If, as a pull-request author, you are aware of a particular reviewer who really should be involved in this particular change, you are free to remove one of the auto-assigned reviewers and manually assign the necessary specialist.

### Review Timeframe
Maintainers should attempt to look at any PR assigned to them within a 24 hour working period. If they are unable to do so, they should contact the PR author informing them that they will not be able to provide a review in a timely manner.
