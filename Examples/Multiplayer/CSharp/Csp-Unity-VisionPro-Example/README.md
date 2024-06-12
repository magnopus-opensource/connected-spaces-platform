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

Open the project on the visionPro platform.

![Vision Pro Platform](./Documentation~/Images/howto_platform.png)

Open the _Example_ Scene.

![Example Scene](./Documentation~/Images/howto_scene.png)

Press the Play button to start the editor runtime.

![Play](./Documentation~/Images/howto_play.png)

Sign Up if you don't already have an account.

![Sign Up](./Documentation~/Images/howto_signup.png)

Confirm Email if so.

![Email Confirm](./Documentation~/Images/howto_email.png)

Sign In with a confirmed, existing account.

![Sign In](./Documentation~/Images/howto_signin.png)

Create a Space if you don't have one already (Give it a name).

![Create Space](./Documentation~/Images/howto_space.png)

Enter an existing space. Move around using W/A/S/D, rotate using Q & E.

![Enter Space](./Documentation~/Images/howto_enter.png)

(Optional) With another copy of the project, sign in using a different account and see the other player appear and move around in real-time.

![Enter Space](./Documentation~/Images/howto_multiplayer.png)



## Run on visionOS Simulator:

In Unity, build for the VisionOS Simulator (Target SDK).

![Enter Space](./Documentation~/Images/howto_build_sim.png)

Run on VisionOS Simulator 1.1+.

![Enter Space](./Documentation~/Images/howto_run_sim.png)

With multiple users signed into a space (eg: one using the Unity editor), the remote players can be seen moving around.

![Enter Space](./Documentation~/Images/howto_multiplayer_sim.png)


## Run on visionOS Device:

In Unity, build for VisionOS Device.

![Enter Space](./Documentation~/Images/howto_build_device.png)

Check that the signing for Device is set up properly using a valid profile / account.

![Signing](./Documentation~/Images/howto_sign_device.png)

Run on visionOS Device.

![Run](./Documentation~/Images/howto_run_device.png)

With multiple users signed into a space (eg: one using the Unity editor), the remote players can be seen moving around.

![Enter Space](./Documentation~/Images/howto_multiplayer_device.png)


## References:
https://docs.unity3d.com/Manual/visionOS.html