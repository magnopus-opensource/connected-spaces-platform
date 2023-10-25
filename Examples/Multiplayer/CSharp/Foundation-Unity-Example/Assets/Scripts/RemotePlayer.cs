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
            UnityEngine.Vector3 updatedPos = Entity.GetPosition().ToUnityVector().ToUnityPositionFromGLTF();
            UpdatePosition(updatedPos);
        }
    }

    private void UpdatePosition(UnityEngine.Vector3 updatedPos)
    {
        transform.position = updatedPos;
    }

    private void OnEntityUpdate(object sender, (SpaceEntity arg1, SpaceEntityUpdateFlags arg2, Array<ComponentUpdateInfo> arg3) eventArgs)
    {
        if (Entity.GetId() != eventArgs.arg1.GetId())
        {
            return;
        }

        if (eventArgs.arg2.HasFlag(SpaceEntityUpdateFlags.UPDATE_FLAGS_POSITION))
        {
            isEntityUpdated = true;
        }
    }

    private void OnEntityDestroyed(object sender, bool success)
    {
        Destroy(this.gameObject);
    }
}
