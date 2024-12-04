# MANUAL

This manual serves as a central hub for documentation, guides, and resources related to the working with CSP.

Whether you're a developer, contributor, or simply curious about the platform, this wiki is here to help you get started either using or contributing to CSP.

## Getting Started

Connected Spaces Platform currently provides APIs for 3 languages: C++, C#, and TypeScript/JavaScript.
For a quick guide on getting started with Connected Spaces Platform, follow the link below that corresponds to the language you use:

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   getting_started/cplusplus
   getting_started/csharp
   getting_started/javascript
```

## Tutorials

Use these tutorials to help you get yor CSP-enabled application running as fast as possible.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   tutorials/cplusplus
   tutorials/csharp
   tutorials/javascript
```

## Processes, Principles and Tooling

Documentation on processes and principles that the library adheres to.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   documentation/programming_guidelines
   documentation/adding_components
   documentation/branching_strategy
   documentation/wrapper_generator_specs
   documentation/script_system
   documentation/expected_application_flow
   documentation/multiplayer_state_replication
   documentation/services_abstraction_layer
   documentation/external_tools
```

## Building CSP

If you're forking CSP for your own purposes, or seeking to contribute back, you can find documentation on how to build CSP here.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   building/web
   building/cpp
   building/csharp
   building/android
   building/macos
   building/ios
```

## Tests

CSP's functionality is validated via an ever-growing suite of tests, and any chance introducing new functionality is expected to also include test coverage.
In this section, you can find out how the tests work, testing best practices, and how to run them.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   tests/best_practices
   tests/test_locations
   tests/test_account_credentials
```

## Debugging

Inevitably, at some point you will encounter unexpected behavior in your application, and you'll break out the debugger to diagnose.

CSP itself is also debuggable, and in this section you can learn how to dive inside the library at runtime.

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   debugging/debugging_unity
   debugging/debugging_web
   debugging/performance_profiling
```
