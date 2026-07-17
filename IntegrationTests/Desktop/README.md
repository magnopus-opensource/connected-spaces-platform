# Integration Tests

These integration tests verify that the installed CSP package works correctly by linking against it and calling a single function.

---

## Prerequisites

Before running these tests, ensure:

- CSP has been built and installed (see build-system docs)
- Ensure you are in the IntegrationTests/Desktop directory

---

## Running Integration Tests

1. **Install dependencies**

   Run Conan using the same configuration and profile used to build CSP:

    ```sh
   conan install ./ \
     -s build_type=<CONFIG> \
     --build=missing \
     --profile:host=../../profiles/host/<PROFILE>
    ```

2. **Configuring the Build**

    - Use CMake to configure the build.

    ```sh
    cmake -S . -B <BUILD_DIR> -DCMAKE_TOOLCHAIN_FILE=<GENERATED_CONAN_TOOLCHAIN> -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    ```

3. **Build the Tests**
    ```sh
    cmake --build <BUILD_DIR> --config <CONFIG>
    ```

4. **Run the Tests**
    ```sh
    ctest --test-dir <BUILD_DIR> -C <CONFIG> -V
    ```