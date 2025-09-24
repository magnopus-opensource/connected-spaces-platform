using CppSharp;
using CppSharp.AST;
using CppSharp.Generators;
using CppSharp.Passes;

class SampleLibrary : ILibrary
{
    public void Setup(Driver driver)
    {
        var options = driver.Options;
        options.GeneratorKind = GeneratorKind.CSharp;
        var module = options.AddModule("Foo");
        module.IncludeDirs.Add(@"C:\dev\connected-spaces-platform\CppSharpApp\CppSharpPrototypeApp\TestCppLibFoo");
        module.Headers.Add("Foo.h");
        module.LibraryDirs.Add(@"C:\dev\connected-spaces-platform\CppSharpApp\CppSharpPrototypeApp\TestCppLibFoo\build\Release");
        module.Libraries.Add("Foo.lib");
    }

    public void SetupPasses(Driver driver)
    {
        driver.Context.TranslationUnitPasses.RenameDeclsUpperCase(RenameTargets.Any);
        driver.Context.TranslationUnitPasses.AddPass(new FunctionToInstanceMethodPass());
    }

    public void Preprocess(Driver driver, ASTContext ctx)
    {

    }

    public void Postprocess(Driver driver, ASTContext ctx)
    {

    }
}

class Program
{
    static void Main(string[] args)
    {
        Console.WriteLine("Running");
        ConsoleDriver.Run(new SampleLibrary());
    }
}