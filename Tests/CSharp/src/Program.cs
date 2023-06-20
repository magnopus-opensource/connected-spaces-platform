using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;

using static CSharpTests.TestHelper;


namespace CSharpTests
{
    class Program
    {
        static void Main(string[] args)
        {
            LogDebug($"Foundation Build ID: {Csp.CSPFoundation.GetBuildID()}");

            TestResultWriterBase resultWriter = null;

            foreach (var arg in args)
            {
                var opt = arg.Substring(0, arg.IndexOf('='));

                if (opt == "--gtest_output")
                {
                    var value = arg.Substring(arg.IndexOf('=') + 1);
                    var outputType = value.Substring(0, value.IndexOf(':'));
                    var outputPath = value.Substring(value.IndexOf(':') + 1);

                    if (outputType == "xml")
                        resultWriter = new XMLTestResultWriter(outputPath);
                }
            }

            var suites = new Dictionary<string, Dictionary<string, Action>>();
            var asm = Assembly.GetExecutingAssembly();
            
            foreach (var type in asm.DefinedTypes)
            {
                if (!type.IsClass)
                    continue;

                var tests = new Dictionary<string, Action>();

                foreach (var method in type.DeclaredMethods)
                {
                    var attr = method.GetCustomAttribute<TestAttribute>();

                    if (attr != null)
                        tests[method.Name] = Expression.Lambda<Action>(Expression.Call(method)).Compile();
                }

                if (tests.Count > 0)
                    suites[type.FullName] = tests;
            }

            var suiteCount = suites.Count;
            var testCount = suites.Aggregate(0, (count, suite) => count + suite.Value.Count);


            Log("[==========]", ConsoleColor.Green, $"Running {testCount} test{(testCount > 1 ? "s" : "")} from {suiteCount} test suite{(suiteCount > 1 ? "s" : "")}.");

            var totalElapsed = 0L;
            var passedCount = 0;
            var failed = new List<string>();

            var sw = new Stopwatch();

            foreach (var suite in suites)
            {
                var suiteName = suite.Key;
                var tests = suite.Value;
                var suiteElapsed = 0L;

                Log("[----------]", ConsoleColor.Green, $"{tests.Count} test{(tests.Count > 1 ? "s" : "")} from {suiteName}");
                
                resultWriter?.AddTestSuite(suiteName);

                foreach (var test in tests)
                {
                    var testName = test.Key;
                    var func = test.Value;

                    Log("[ RUN      ]", ConsoleColor.Green, $"{suiteName}.{testName}");

                    var startTime = DateTime.Now;
                    var passed = false;

                    sw.Reset();
                    sw.Start();

                    StartupTest();

                    try
                    {
                        func();
                        passed = true;
                    }
                    catch (AssertFailedException e)
                    {
                        // TODO: Add error message to test result xml output
                        Log("[     FAIL ]", ConsoleColor.Red, $"Assert failed at {e.FilePath}:{e.Line} ({e.Method}). {e.Description}.");
                    }
                    catch (Exception e)
                    {
                        // TODO: Add error message to test result xml output
                        LogError($"{e.Message}\nStack trace:\n{e.StackTrace}");

                        if (Debugger.IsAttached)
                            throw;
                    }

                    bool cleanedUp = false;

                    while (!cleanedUp)
                    {
                        try
                        {
                            CleanupTest();
                            cleanedUp = true;
                        }
                        catch (AssertFailedException e)
                        {
                            // TODO: Add error message to test result xml output
                            Log("[     FAIL ]", ConsoleColor.Red, $"Assert failed at {e.FilePath}:{e.Line} ({e.Method})");
                            passed = false;
                        }
                        catch (Exception e)
                        {
                            // TODO: Add error message to test result xml output
                            LogError(e.Message);
                            passed = false;

                            if (Debugger.IsAttached)
                                throw;
                        }
                    }

                    sw.Stop();

                    var elapsed = sw.ElapsedMilliseconds;

                    if (passed)
                    {
                        Log("[       OK ]", ConsoleColor.Green, $"{suiteName}.{testName} ({elapsed} ms)");
                        passedCount++;
                    }
                    else
                    {
                        Log("[     FAIL ]", ConsoleColor.Red, $"{suiteName}.{testName} ({elapsed} ms)");
                        failed.Add($"{suiteName}.{testName}");
                    }

                    suiteElapsed += elapsed;

                    resultWriter?.AddTestCase(suiteName, testName, startTime, elapsed, passed);
                }

                Log("[----------]", ConsoleColor.Green, $"{tests.Count} test{(tests.Count > 1 ? "s" : "")} from {suiteName} ({suiteElapsed} ms total)\n");
                totalElapsed += suiteElapsed;
            }

            Console.WriteLine();

            Log("[==========] ", ConsoleColor.Green, $"{testCount} test{(testCount > 1 ? "s" : "")} from {suiteCount} test suite{(suiteCount > 1 ? "s" : "")} ran. ({totalElapsed} ms total)");

            if (passedCount > 0)
                Log("[  PASSED  ]", ConsoleColor.Green, $"{passedCount} test{(passedCount > 1 ? "s" : "")}.");

            if (failed.Count > 0)
            {
                Log("[  FAILED  ]", ConsoleColor.Red, $"{failed.Count} test{(failed.Count > 1 ? "s" : "")}, listed below:");

                foreach (var name in failed)
                    Log("[  FAILED  ]", ConsoleColor.Red, name);
            }

            resultWriter?.WriteResults();

            if (Debugger.IsAttached)
            {
                Console.WriteLine("\nPress any key to close this window . . .");
                Console.ReadKey();
            }
        }
    }
}
