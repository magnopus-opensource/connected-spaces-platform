# foundation-c#-Unity-example

## Setup and Running
This example is set up as a Unity project. Using Unity version 2022.3.19f1, you can open the folder and try out the solution.

To run:

1. Open solution with with Unity 2022.3.19f1 apple silicon version and visionOS platform.
2. Set credentials in the HelloWorld login function.
3. Set a space id in the AccountUI.EnterSpace function.
4. Put the local visionOS CSP libraries in the Packages/csp/visionOS folder.
5. Open Scene 'Example'.
6. Press Play button in editor.
7. If you want to build it, make sure the build target from Player Settings/ Build settings is set to device or simulator depending on your goal.

## Example contains: 
- Start, Tick, Stop Foundation
- Sign up, Log in, Log out
- Find, Create, Enter, Exit, Delete Space
- Create Avatar for local player
- Provide the ability to move local avatar and send entity position updates
- Process received remote player entity position updates
