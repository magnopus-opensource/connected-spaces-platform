# External Tools

## Clang Format
The formatter used in the project is Clang Format, the configuration file is contained at the root of the repository, ".clang-format".
Clang is clever in the way that it searches for the format file upto the root of the project and applies it throughout, meaning it is simple to configure once and just run the tool.

There is a Visual Studio Extension [available ](https://marketplace.visualstudio.com/items?itemName=LLVMExtensions.ClangFormat&ssr=false#overview).

You can run Clang Format on the whole project by using [this python script](https://github.com/magnopus-opensource/connected-spaces-platform/blob/main/Tools/Formatter/RunFormatter.py). Simply run this from anywhere and it will apply to the whole project.

To create a runnable external tool, simply go to **Tools -> External Tools... -> Add**
>  - Title: Clang-Format File
>  - Command: C:\Program Files\LLVM\bin\clang-format.exe
>  - Arguments: -i "$(ItemPath)"
>  - Use Output Window: Ticked

## Cppcheck

**Cppcheck** is a powerful and popular open-source static analysis tool for C and C++ code. Unlike a compiler, which primarily checks for syntax errors, Cppcheck focuses on detecting bugs and dangerous coding constructs that compilers often miss. Its core philosophy is to produce very few false positives, allowing developers to trust the warnings it generates and address genuine issues.

Key features include:
- Detection of memory leaks, buffer overflows, and uninitialized variables.
- Identification of style, portability, and performance-related issues.
- A command-line interface for seamless integration into build systems.
- A dedicated GUI application for reviewing analysis reports.

For more information and a complete list of checks, refer to the official Cppcheck resources.

**Useful Links:**
- **Official Website:** [https://cppcheck.sourceforge.io/](https://cppcheck.sourceforge.io/)
- **Manual & Documentation:** [https://cppcheck.sourceforge.io/manual.pdf](https://cppcheck.sourceforge.io/manual.pdf)

### Static Analysis in Connected Spaces Platform

Cppcheck is not currently an enforced part of the Connected Spaces Platform, it is an optional tool that can be used to assist developers in maintaining code quality.

The primary goal of using Cppcheck is to:
- **Improve Code Reliability:** Automatically detect common programming errors that can lead to crashes or undefined behavior.
- **Enforce Coding Standards:** Identify deviations from best practices and maintain consistency across the codebase.
- **Reduce Debugging Time:** Catch issues at compile time, reducing the need for lengthy debugging sessions later.
- **Enhance Code Portability:** Point out potential issues when compiling on different platforms or with different compilers.

#### Configuration Details

A convenient batch script, ```run_cppcheck_static_analysis.bat```, is provided in the project root to automate the analysis with the correct configuration. To run Cppcheck, simply execute this script from your command line:

```
run_cppcheck_static_analysis.bat
```

Alternatively, Cppcheck can be configured to run with the following command-line arguments to define the analysis scope and output format:

```
"C:\Program Files\Cppcheck\cppcheck.exe" --xml --xml-version=2 --output-file=cppcheck_output.xml --enable=all --suppress=unknownMacro -I"Library/include" Library
```

A breakdown of the key configuration flags is as follows:

- `"C:\Program Files\Cppcheck\cppcheck.exe"`: This specifies the full path to the Cppcheck executable.
- `--xml` and `--xml-version=2`: The analysis results are output in an XML format. This is specifically chosen for easy integration with external tools and for viewing with dedicated applications.
- `--output-file=cppcheck_output.xml`: The report is saved to a fixed file named `cppcheck_output.xml` in the current working directory.
- `--enable=all`: This flag enables all available checks, including style, performance, portability, and unused function checks, for a comprehensive analysis.
- `--suppress=unknownMacro`: This flag prevents Cppcheck from reporting warnings for macros that are not defined in the source but are provided by the compiler or build environment. This helps to reduce noise and focus the report on more critical issues.
- `-I"Library/include"`: This specifies the include path Cppcheck should use to resolve headers. In this case, it's a fixed path relative to the current directory.
- `Library`: This specifies the source directory to be analyzed. In this configuration, it is a fixed directory named `Library`.

#### How to View Results

The generated XML report, located at `cppcheck_output.xml`, contains a detailed list of all warnings and errors. This file can be viewed in a few different ways:

- **Cppcheck GUI:** Cppcheck provides a dedicated graphical user interface (GUI) application that can open and display the XML report. This is an excellent way to browse the findings visually, as it highlights the issues and provides detailed descriptions. The GUI application can be downloaded from the official website.
- **Text Editor:** The XML file can also be opened in any text editor, though this is not recommended for large reports as it can be difficult to read.
