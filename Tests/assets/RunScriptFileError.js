// Test script for ScriptSystemTests.RuntimeErrorInScriptFileIsReported.
// Calling a value that isn't a function throws "TypeError: not a function".
const NotAFunction = 42;
NotAFunction();