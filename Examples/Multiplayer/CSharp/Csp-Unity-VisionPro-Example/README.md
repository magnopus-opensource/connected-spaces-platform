# CSP-Unity-VisionPro-Example

This example shows how to test out various multiplayer features: in editor, in the visionOS simulator, and on a visionOS device.

## Example contains:
- Start, Tick, Stop CSP
- Sign up, Log in, Log out
- Find, Create, Enter, Exit, Delete Space
- Create Avatar for local player
- Provide the ability to move local avatar and send entity position updates
- Process received remote player entity position updates


## Requirements:

- Unity 2022.3.25f1
- XCode 15.4
- Vision Pro Simulator 1.1+
- Vision Pro Device (Optional)


## Run in Editor:

Open the project on the visionPro platform

![Vision Pro Platform](./Images/howto_platform.png)
<img src="./Images/howto_platform.png" />

Open the _Example_ Scene

![Example Scene](./Images/howto_scene.png)

<img src="./Images/howto_scene.png" />

Press the Play button to start the editor runtime.


Sign Up if you don't already have an account

![Sign Up](./Images/howto_signup.png)

Confirm Email if so.

![Email Confirm](./Images/howto_email.png)

Sign In with a confirmed, existing account

![Sign In](./Images/howto_signin.png)

Create a Space if you don't have one already (Give it a name)

![Create Space](./Images/howto_space.png)

Enter an existing space. Move around using W/A/S/D, rotate using Q & E.

![Enter Space](./Images/howto_enter.png)

(Optional) With another copy of the project, sign in using a different account and see the other player appear and move around in real-time.

![Enter Space](./Images/howto_multiplayer.png)



## Run on visionOS Simulator:

Build for VisionOS Simulator

![Enter Space](./Images/howto_build_sim.png)

Run on VisionOS Simulator 1.1+

![Enter Space](./Images/howto_run_sim.png)

With multiple users signed into space (eg one using Unity editor), the remote players can be seen moving around.

![Enter Space](./Images/howto_multiplayer_sim.png)


## Run on visionOS Device:

Build for VisionOS Device

![Enter Space](./Images/howto_build_device.png)

Check that the signing for Device is set up properly using a valid profile / account

![Enter Space](./Images/howto_signing_device.png)

Run on VisionOS Device

![Enter Space](./Images/howto_run_device.png)

With multiple users signed into space (eg one using Unity editor), the remote players can be seen moving around.

![Enter Space](./Images/howto_multiplayer_device.png)


## References:
https://docs.unity3d.com/Manual/visionOS.html