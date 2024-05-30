// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using UnityEngine;
using CspCommon = Csp.Common;

public static class TransformUtils
{
    // https://github.com/KhronosGroup/UnityGLTF/blob/master/UnityGLTF/Assets/UnityGLTF/Runtime/Scripts/Extensions/SchemaExtensions.cs
    // GLTF coordinate (right handed): -x = right, y = up, z = forward
    private static readonly Vector3 GLTFCoordinateSpaceConversionScale = new Vector3(-1, 1, 1);

    /// <summary>
    /// Converts a given <seealso cref="CspCommon.Vector3" /> to <seealso cref="UnityEngine.Vector3" />.
    /// </summary>
    /// <param name="vec"></param>
    /// <returns></returns>
    public static Vector3 ToUnityVector(this CspCommon.Vector3 vec)
    {
        return new Vector3(vec.X, vec.Y, vec.Z);
    }

    /// <summary>
    /// Converts a given <seealso cref="UnityEngine.Vector3" /> to <seealso cref="CspCommon.Vector3" />. 
    /// </summary>
    /// <param name="vec"></param>
    /// <returns></returns>
    public static CspCommon.Vector3 ToCSPVector(this Vector3 vec)
    {
        return new CspCommon.Vector3(vec.x, vec.y, vec.z);
    }

    /// <summary>
    /// Converts a given <seealso cref="CspCommon.Vector4" /> to <seealso cref="UnityEngine.Quaternion" />.
    /// </summary>
    /// <param name="vec"></param>
    /// <returns></returns>
    public static Quaternion ToUnityQuaternion(this CspCommon.Vector4 vec)
    {
        return new Quaternion(vec.X, vec.Y, vec.Z, vec.W);
    }

    /// <summary>
    /// Converts a given position in GLTF (right-handed, Y-up, Z-forward) coordinate 
    /// to Unity (left-handed, Y-up, Z-forward) coordinate.
    /// </summary>
    /// <param name="gltfPosition"></param>
    /// <returns></returns>
    public static Vector3 ToUnityPositionFromGLTF(this Vector3 gltfPosition)
    {
        // Convert from GLTF coordinate (right handed)
        // -x = right
        // y = up
        // z = forward

        var unityPosition = Vector3.Scale(gltfPosition, GLTFCoordinateSpaceConversionScale);
        return unityPosition;
    }

    /// <summary>
    /// Converts a given position in Unity (left-handed, Y-up, Z-forward) coordinate 
    /// to GLTF (right-handed, Y-up, Z-forward) coordinate
    /// </summary>
    /// <param name="unityPosition"></param>
    /// <returns></returns>
    public static Vector3 ToGLTFPositionFromUnity(this Vector3 unityPosition)
    {
        var gltfPosition = Vector3.Scale(unityPosition, GLTFCoordinateSpaceConversionScale);
        return gltfPosition;
    }

    /// <summary>
    /// Converts a given rotation in GLTF (right-handed, Y-up, Z-forward) coordinate 
    /// to Unity (left-handed, Y-up, Z-forward) coordinate.
    /// </summary>
    /// <param name="gltfRotation"></param>
    /// <returns></returns>
    public static Quaternion ToUnityRotationFromGLTF(this Quaternion gltfRotation)
    {
        // Convert from GLTF coordinate (right handed)
        // -x = right
        // y = up
        // z = forward

        Vector3 fromAxisOfRotation = new Vector3(gltfRotation.x, gltfRotation.y, gltfRotation.z);
        // Flip handness
        float axisFlipScale = -1.0f;
        Vector3 toAxisOfRotation = axisFlipScale * Vector3.Scale(fromAxisOfRotation, GLTFCoordinateSpaceConversionScale);
        var unityRotation = new Quaternion(toAxisOfRotation.x, toAxisOfRotation.y, toAxisOfRotation.z, gltfRotation.w);

        return unityRotation;
    }

    /// <summary>
    /// Converts a given scale in GLTF (right-handed, Y-up, Z-forward) coordinate 
    /// to Unity (left-handed, Y-up, Z-forward) coordinate.
    /// </summary>
    /// <param name="gltfScale"></param>
    /// <returns></returns>
    public static Vector3 ToUnityScaleFromGLTF(this Vector3 gltfScale)
    {
        return gltfScale;
    }
    
    /// <summary>
    /// Converts a GLTF rotated quaternion from unity quaternion
    /// </summary>
    /// <param name="unityRotation"></param>
    /// <returns>Quaternion</returns>
    public static Quaternion ToGLTFRotationFromUnity(this Quaternion unityRotation)
    {
        Vector3 fromAxisOfRotation = new Vector3(unityRotation.x, unityRotation.y, unityRotation.z);
        // Flip handness
        float axisFlipScale = -1.0f;
        Vector3 toAxisOfRotation = axisFlipScale * Vector3.Scale(fromAxisOfRotation, GLTFCoordinateSpaceConversionScale);
        var gltfRotation = new Quaternion(toAxisOfRotation.x, toAxisOfRotation.y, toAxisOfRotation.z, unityRotation.w);

        return gltfRotation;
    }
    
    /// <summary>
    /// Converts a unity Quaternion into a Vector4
    /// </summary>
    /// <param name="vec"></param>
    /// <returns>Vector4</returns>
    public static Csp.Common.Vector4 ToFoundationVector(this Quaternion vec)
    {
        return new Csp.Common.Vector4(vec.x, vec.y, vec.z, vec.w);
    }
}
