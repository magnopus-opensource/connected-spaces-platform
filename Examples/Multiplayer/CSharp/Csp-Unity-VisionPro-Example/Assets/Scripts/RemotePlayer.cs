// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using Csp.Common;
using Csp.Multiplayer;
using System;
using UnityEngine;

public class RemotePlayer : MonoBehaviour
{
    public SpaceEntity Entity { get; private set; }

    private bool isEntityUpdated = false;

    public void Initialize(SpaceEntity entity)
    {
        if (entity == null)
        {
            Debug.LogError("Failed to create Remote Player, entity was null.");
            return;
        }

        Debug.Log("Initializing remote player.");

        Entity = entity;
        Entity.OnUpdate += OnEntityUpdate;
        Entity.OnDestroy += OnEntityDestroyed;
    }

    private void OnDestroy()
    {
        if (Entity == null)
        {
            throw new InvalidOperationException($"Entity was disposed before {nameof(OnDestroy)} was called");
        }

        Entity.OnUpdate -= OnEntityUpdate;
        Entity.OnDestroy -= OnEntityDestroyed;
        Entity.Dispose();
    }

    private void Update()
    {
        // NOTE: if you are using Unity async, instead of using the Update loop to get back onto the main thread
        // you can call 'await AsyncManager.UnitySyncContext;' where you receive the event
        if (isEntityUpdated)
        {
            isEntityUpdated = false;
            using var cspVectorPosition = Entity.GetPosition(); 
            var newPosition = cspVectorPosition.ToUnityVector().ToUnityPositionFromGLTF();
            UpdatePosition(newPosition);
            
            using var cspVectorRotation = Entity.GetRotation();
            var newRotation = cspVectorRotation.ToUnityQuaternion().ToUnityRotationFromGLTF();
            UpdateRotation(newRotation);
        }
    }

    private void UpdatePosition(UnityEngine.Vector3 updatedPosition)
    {
        transform.position = updatedPosition;
    }
    private void UpdateRotation(UnityEngine.Quaternion updatedRotation)
    {
        transform.rotation = updatedRotation;
    }

    private void OnEntityUpdate(object sender, (SpaceEntity arg1, SpaceEntityUpdateFlags arg2, Array<ComponentUpdateInfo> arg3) eventArgs)
    {
        if (Entity.GetId() != eventArgs.arg1.GetId())
        {
            return;
        }

        if (eventArgs.arg2.HasFlag(SpaceEntityUpdateFlags.UPDATE_FLAGS_POSITION) || eventArgs.arg2.HasFlag(SpaceEntityUpdateFlags.UPDATE_FLAGS_ROTATION) )
        {
            isEntityUpdated = true;
        }
    }

    private void OnEntityDestroyed(object sender, bool success)
    {
        Destroy(this.gameObject);
    }
}
