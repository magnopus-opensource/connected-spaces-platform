# Wrapper Generator Specifications

The CSP wrapper generator is responsible for automatically generating wrapper APIs for supported non-C++ languages and is run as part of the pipeline when generating new versions of Foundation.

The wrapper generator allows CSP contributors to focus on building new features and APIs in one language, and it takes care of the work required to expose that to client applications built in other languages. In this way, it also guarantees API consistency and parity across all languages supported by Foundation.

To achieve this, it relies on API source files including some markup/generation-time hints, to allow the wrapper generator to understand the intent behind the source, and render it to other languages appropriately. 

## General rules
* Only classes and functions marked with `CSP_API` are exported by the parser.
* For classes, only public fields and functions are exported by the parser.
  * The only exception to this is constructors/destructors. Metadata _is_ generated for private constructors/destructors and has its `is_private` field set to `True`. This is needed as the wrappers need to know that a class shouldn't be publicly constructible or destructible.
* When mutating the CSP C++ API, ensure any functions that accept enums pass those by parameters by value, not by reference. The wrapper generator does not support passing enums by reference.

## Wrapper-specific rules

```eval_rst
.. toctree::
   :maxdepth: 1
   :titlesonly:

   wrapper_generator/c_interface
   wrapper_generator/csharp_wrapper
   wrapper_generator/typescript_wrapper
```