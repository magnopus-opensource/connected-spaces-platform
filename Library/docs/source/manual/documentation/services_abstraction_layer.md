# Services Abstraction Layer

The Connected Spaces Platform communicates with the world via RESTful endpoints and WebSockets, and in order for CSP to meet its goals around interoperability, it depends on those RESTful endpoints following a spec.

That spec comes in the form of an automatically generated C++ API that can be found in the [csp-services repo](https://github.com/magnopus-opensource/csp-services).

Developers are able to build their own web services that implement this spec, and the implementation can then be validated by directing CSP to interface with those services.

CSP consumes the C++ services API as a **git submodule** referencing a specific commit hash. This allows CSP to electively update which version of the services spec it uses over time.

What that means practically speaking, is that if you as a CSP contributor wish to make use of new features introduced to the services spec, or need to respond to changes to the spec, you will need to update the version of the services repo that the CSP submodule references.

Typically, you will be updating the CSP submodule to reference to the latest version of the services repo. To do that, follow the same process as with any other change, and in your branch invoke:

`git submodule update --init --remote modules/csp-services`

Include the resultant change as part of your push to the remote branch, and include it as part of the PR you eventually create.