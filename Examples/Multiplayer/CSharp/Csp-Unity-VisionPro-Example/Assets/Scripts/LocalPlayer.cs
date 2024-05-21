// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using Csp.Multiplayer;
using System;
using UnityEngine;
using UnityEngine.InputSystem;
using Random = UnityEngine.Random;

public class LocalPlayer : MonoBehaviour
{
    private const float MoveSpeed = 10.0f;
    private const float RotateSpeed = 25.0f;
    /// <summary>
    /// The amount of distance needed to move in order to send a transform update on the network
    /// </summary>
    private const float SendPositionNetworkDistanceInterval = 0.1f;
    /// <summary>
    /// The amount of angle in degrees needed to rotate in order to send a transform on the network
    /// </summary>
    private const float SendPositionNetworkAngleInterval = 1.0f;

    public SpaceEntity Entity { get; private set; }

    private InputAction inputPosition;
    private InputAction inputRotation;

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

    private void Awake()
    {
        inputPosition = new InputAction("devicePosition");
        inputPosition.AddBinding("<XRHMD>/centerEyePosition");
        inputRotation = new InputAction("deviceRotation");
        inputRotation.AddBinding("<XRHMD>/centerEyeRotation");
    }
    
    private void OnEnable()
    {
        inputPosition.Enable();
        inputRotation.Enable();
    }
    
    private void OnDisable()
    {
        inputPosition.Disable();
        inputRotation.Disable();
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
        Vector3 translationValue = Vector3.zero;
        Quaternion rotationValue = transform.rotation;

        if (Application.isEditor)
        {
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
            
            // rotation
            var rotationAxis = Vector3.up;
            var rotationAngle = RotateSpeed * Time.deltaTime;
            if (Input.GetKey(KeyCode.E))
            {
                rotationValue *= Quaternion.AngleAxis(-rotationAngle,rotationAxis);
            }
            else if (Input.GetKey(KeyCode.Q))
            {
                rotationValue *= Quaternion.AngleAxis(rotationAngle,rotationAxis);
            }
            
            translationValue = transform.position + (movementInput * Time.deltaTime * MoveSpeed);
        }
        else if(Application.platform == RuntimePlatform.VisionOS)
        {
            // Get position & rotation input from device
            translationValue = inputPosition.ReadValue<Vector3>();
            rotationValue = inputRotation.ReadValue<Quaternion>();
            
            // Project rotation to xz 2D plane (to appear as a typical user)
            Vector3 forward3D = rotationValue * Vector3.forward; // 1,0,0
            Vector3 forward2DPlane = Vector3.ProjectOnPlane(forward3D, Vector3.up);
            float angle2DPlane = Vector3.SignedAngle(Vector3.forward, forward2DPlane, Vector3.up);
            rotationValue = Quaternion.AngleAxis(angle2DPlane, Vector3.up);
            
            // Force user y value to 0 - keep out of immediate visual range, but allow for feedback  
            translationValue.y = 0.0f;
        }
        
        MovePlayerTo(translationValue, rotationValue);
    }

    private void MovePlayerTo(Vector3 nextPosition, Quaternion nextRotation)
    {
        transform.position = nextPosition;
        transform.rotation = nextRotation;

        Vector3 entityPosition = Entity.GetPosition().ToUnityVector().ToUnityPositionFromGLTF();
        Quaternion entityRotation = Entity.GetRotation().ToUnityQuaternion().ToUnityRotationFromGLTF();
        float distanceDifference = Vector3.Distance(transform.position, entityPosition);
        
        // Get 3 axis angles differences from rotation 
        
        Vector3 entityUp = entityRotation * Vector3.up;
        Vector3 transformUp = transform.rotation * Vector3.up;
        float angleUp = Vector3.Angle(entityUp, transformUp);
        
        Vector3 entityRight = entityRotation * Vector3.right;
        Vector3 transformRight = transform.rotation * Vector3.right;
        float angleRight = Vector3.Angle(entityRight, transformRight);
        
        Vector3 entityForward = entityRotation * Vector3.forward;
        Vector3 transformForward = transform.rotation * Vector3.forward;
        float angleForward = Vector3.Angle(entityForward, transformForward);
        
        // average angle between axes
        float distanceRotation = (angleUp + angleForward + angleRight) / 3.0f; 
        
        // Send network event
        // NOTE: you may not want to send a transform update every frame (so I use difference from local transform)
        if (distanceDifference > SendPositionNetworkDistanceInterval || distanceRotation > SendPositionNetworkAngleInterval)
        {
            using var cspVectorPosition = nextPosition.ToGLTFPositionFromUnity().ToCSPVector();
            using var cspQuatRotation = nextRotation.ToGLTFRotationFromUnity().ToFoundationVector();
                        
            Entity.SetPosition(cspVectorPosition);
            Entity.SetRotation(cspQuatRotation);
            Entity.QueueUpdate();
        }
    }
}
