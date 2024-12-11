# C# (Unity)

## Package Installation Steps

1. Navigate to your project's `Project Settings`.
2. Select the `Package Manager` option.
3. Add a new Scoped Registry with the following details:

> **Name** `npmjs`  
> **URL** [https://registry.npmjs.org](https://registry.npmjs.org)  
> **Scope(s)** `connected-spaces-platform`  

4. Save the new scoped registry.

![image info](../../_static/getting_started/npmjs_registry.png)

5. Navigate to the Package Manager window via `Window->Package Manager`
6. From the Packages dropdown, select the `My Registries` option.

![image info](../../_static/getting_started/myregistry.png)

7. From the `Other` category, selected the `connected-spaces-platform.unity.core` package.
8. Select `Install` to add the latest version of the package to your project.

![image info](../../_static/getting_started/install_package.png)

## VisionOS

The Vision Pro runs on Apple silicion and does not allow for applications built on Intel architectures to run on it. As a result, CSP binaries available in published packages are all built explicitly for Apple silicon.

This means that the Unity CSP application developer must also be using a version of Unity Editor for Apple silicon.
