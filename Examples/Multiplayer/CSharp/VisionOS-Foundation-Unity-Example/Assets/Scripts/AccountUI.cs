// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using System;
using UnityEngine;
using UnityEngine.UI;
using Random = System.Random;

public class AccountUI : MonoBehaviour
{
    public event Action<string, string> OnSignUp;
    public event Action<string, string> OnSignIn;
    public event Action<string, bool> OnEnterSpace;
    public event Action<string> OnCreateSpace;
    public event Action OnDeleteSpace;

    [Header("Sign up and Sign In")]
    [SerializeField] private InputField signUpEmail;
    [SerializeField] private InputField signUpPassword;
    [SerializeField] private InputField signInEmail;
    [SerializeField] private InputField signInPassword;
    [SerializeField] private Button signUpButton;
    [SerializeField] private Button signInButton;

    [Header("Create Space")] [SerializeField]
    private InputField createSpaceName;
    [SerializeField] private Button createSpaceButton;
    [SerializeField] private Button deleteSpaceButton;

    [Header("Enter Space")] [SerializeField]
    private InputField spaceId;
    [SerializeField] private Button useCreateSpaceButton;
    [SerializeField] private Button enterSpaceButton;

    private void Awake()
    {
        signUpButton.onClick.AddListener(SignUp);
        signInButton.onClick.AddListener(SignIn);
        createSpaceButton.onClick.AddListener(CreateSpace);
        useCreateSpaceButton.onClick.AddListener(UseCreatedSpace);
        enterSpaceButton.onClick.AddListener(EnterSpace);
        deleteSpaceButton.onClick.AddListener(DeleteSpace);
    }

    private void EnterSpace()
    {
        //OnEnterSpace?.Invoke(spaceId.text, false);
        Debug.Log("EnterSpace pressed");
        //Replace with your own space until input fields are working
        OnEnterSpace?.Invoke("example space id", false);
    }

    private void UseCreatedSpace()
    {
        OnEnterSpace?.Invoke(String.Empty, true);
    }

    private void CreateSpace()
    {
        // Replaced spacename with random numbers until input fields are working
        var spacename = $"{new Random().Next(10000, 99999)}{new Random().Next(10000, 99999)}";
        OnCreateSpace?.Invoke(spacename);
    }

    private void SignUp()
    {
        OnSignUp?.Invoke(signUpEmail.text, signUpPassword.text);
    }

    private void SignIn()
    {
        OnSignIn?.Invoke(signInEmail.text, signInPassword.text);
    }

    private void DeleteSpace()
    {
        OnDeleteSpace?.Invoke();
    }

    private void OnDestroy()
    {
        signUpButton.onClick.RemoveListener(SignUp);
        signInButton.onClick.RemoveListener(SignIn);
        createSpaceButton.onClick.RemoveListener(CreateSpace);
        useCreateSpaceButton.onClick.RemoveListener(UseCreatedSpace);
        enterSpaceButton.onClick.RemoveListener(EnterSpace);
        deleteSpaceButton.onClick.RemoveListener(DeleteSpace);
    }
}