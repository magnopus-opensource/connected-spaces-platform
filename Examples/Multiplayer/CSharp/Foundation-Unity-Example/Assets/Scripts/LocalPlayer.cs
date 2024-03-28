using Csp.Multiplayer;
using System;
using UnityEngine;

public class LocalPlayer : MonoBehaviour
{
    private const float MoveSpeed = 10.0f;
    /// <summary>
    /// The amount of distance needed to move in order to send a position on the network
    /// </summary>
    private const float SendPositionNetworkDistanceInterval = 0.5f;

    public SpaceEntity Entity { get; private set; }

    private float distanceTraveled = 0;

    public void Initialize(SpaceEntity entity)
    {
        if (entity == null)
        {
            Debug.LogError("Failed to create Local Player, entity was null.");
            return;
        }

        Debug.Log("Initializing local player");
        Entity = entity;
    }

    private void Update()
    {
        CheckInput();
    }

    private void OnDestroy()
    {
        if (Entity == null)
        {
            throw new InvalidOperationException($"Entity was disposed before {nameof(OnDestroy)} was called");
        }

        Entity.Dispose();
    }

    private void CheckInput()
    {
        Vector3 movementInput = Vector3.zero;

        // Vertical movement Input
        if (Input.GetKey(KeyCode.W))
        {
            movementInput.z = 1;
        }
        else if (Input.GetKey(KeyCode.S))
        {
            movementInput.z = -1;
        }

        // Horizontal movement Input
        if (Input.GetKey(KeyCode.D))
        {
            movementInput.x = 1;
        }
        else if (Input.GetKey(KeyCode.A))
        {
            movementInput.x = -1;
        }

        MovePlayer(movementInput);
    }

    private void MovePlayer(Vector3 movementInput)
    {
        var nextPosition = transform.position + (movementInput * Time.deltaTime * MoveSpeed);
        distanceTraveled += Vector3.Distance(transform.position, nextPosition);
        transform.position = nextPosition;

        // Send network event
        // NOTE: you may not want to send a position update every frame (so I use distance traveled)
        if (distanceTraveled > SendPositionNetworkDistanceInterval)
        {
            distanceTraveled = 0.0f;

            using var cspVectorPosition = nextPosition.ToGLTFPositionFromUnity().ToCSPVector();
            Entity.SetPosition(cspVectorPosition);

            Entity.QueueUpdate();
        }
    }
}
