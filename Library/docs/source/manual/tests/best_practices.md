# Testing Best Practices

Based on the philosophy that testable code is readable code, all code should be written in a way that makes testing trivial. Ideally this means small units that can be tested with unit testing that does not rely on making API calls. This obviously misses places that require calls to external APIs so code should be structured in such a way that as much code as possible is testable without relying on the API call and code that does call APIs simply makes those calls and delegates to other functions to handle preparation and results of requests.

Functional tests should test the system end to end via the public API and ensure coverage of any and all external API calls. It is important that functional tests do not simply take the happy path but also test various different combinations of parameters, especially ensuring that invalid parameters are rejected.

It is accepted that in some cases targeting small easily testable functions can be a contradictory goal to performance optimization. In those cases, it is up to the contributor to balance these goals. For example, an action like creating a space which is called infrequently does not need to take micro optimizations into account in the way that a deserialization that occurs on every tick might. In these cases an effort should still be made to have as much unit test coverage as possible but that additional functional tests may be needed to cover all areas. 

This document mostly focuses on C++ code and tests as it is assumed that WrapperGenerator unit tests ensure consistency of the API in the Web/C#. However, in some cases macros are used to control code for different builds and it is important that when this occurs we ensure that functional tests exist targeting all code variants. An example could be the WebClient implementation used in web vs C++.

## Pull requests
When reviewing a pull request it should be clear from tests alone what has been introduced into the code base. If new functionality is added then tests should cover those use cases. If a bug is being fixed a test should be added that reproduces the issue and the new code should ensure that test passes. Sometimes it is not always possible to do this but a PR should not be approved without these in place or a reasonable explanation of why they are missing. 
