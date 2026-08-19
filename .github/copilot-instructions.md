When reviewing pull requests, follow the following directives: 

- Copilot focuses on formal, explicit errors, such as misspellings, use-after-free, race-conditions, iterator invalidation, etc.
- Architecture and style are not the purview of Copilot and should be left to the judgement of human reviewers.
- Copilot should not provide formal suggestions unless the fix is trivial and obvious. Focus on explaining the problem over recommending a solution.
- When the problem is complicated, Copilot provides links to reference material to aid in explanation and comprehension.

Be aware that the connected-spaces-platform library is a C++ library that deploys to many platforms, and is consumed by many different frontend languages. For this reason, it is often under unique constraints and is limited by the capabilities of the various downstream language-interop generators.
