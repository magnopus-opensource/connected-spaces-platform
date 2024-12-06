# Debugging CSP for Web

This guide explains how to debug Foundation for Web using Google Chrome.

## Steps
1. Install the [C/C++ DevTools Support (DWARF)](https://chrome.google.com/webstore/detail/cc%20%20-devtools-support-dwa/pdcpmagijalfljmkmjngeonclgbbannb) Chrome extension.
2. Enable DWARF support for WASM debugging.
    - Open the developer console (ctrl + shift + I).
    - Open the settings panel (F1).
    - Select `Experiments` in the side panel.
    - Check `WebAssembly Debugging: Enable DWARF support`.
3. Go to the settings page for C/C++ DevTools Support (chrome-extension://pdcpmagijalfljmkmjngeonclgbbannb/ExtensionOptions.html).
4. Under `Path substitutions`, click `Add path substitution`.
5. In the first text box, enter `/src/`.
6. In the second text box, enter the path to your local CSP repository clone.
7. Navigate to your web client application that uses CSP in Chrome.
8. Open the developer console (ctrl + shift + I).
9. Wait for a message similar to `[C/C++ DevTools Support (DWARF)] Loaded debug symbols for https://odev.magnoverse.space/b95515fc8363ec3f.wasm, found 1382 source file(s)`.
10. Click the `Sources` tab in the developer console.
11. Navigate to any file by expanding `file://`, then `<your_csp_repo_path>` in the Page sidebar on the left and locating your file, or by pressing Ctrl + P and typing the file name.
12. You can now set any breakpoints you want and inspect call stack and variable values on the right.

## Tips
When you're working with a locally built web client and you would want to pull a fresh FDN package, this command needs to be ran. You'll need to replace the `1.2.3` with the version number you want to be pulled.

`yarn remove @magnopus-opensource/connected-spaces-platform.web && yarn add @magnopus-opensource/connected-spaces-platform.web@1.2.3`