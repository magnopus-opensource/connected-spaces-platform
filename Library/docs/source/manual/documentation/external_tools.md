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