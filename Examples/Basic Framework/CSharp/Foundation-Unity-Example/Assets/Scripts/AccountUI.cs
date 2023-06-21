// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using System;
using UnityEngine;
using UnityEngine.UI;

public class AccountUI : MonoBehaviour
{
    public event Action<string, string> OnSignUp;
    public event Action<string, string> OnSignIn;
    public event Action<string, bool> OnEnterSpace;
    public event Action<string> OnCreateSpace;
    public event Action OnExitSpace;
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
    [SerializeField] private Button exitSpaceButton;

    private void Awake()
    {
        signUpButton.onClick.AddListener(SignUp);
        signInButton.onClick.AddListener(SignIn);
        createSpaceButton.onClick.AddListener(CreateSpace);
        useCreateSpaceButton.onClick.AddListener(UseCreatedSpace);
        enterSpaceButton.onClick.AddListener(EnterSpace);
        deleteSpaceButton.onClick.AddListener(DeleteSpace);
        exitSpaceButton.onClick.AddListener(ExitSpace);
    }

    private void EnterSpace()
    {
        OnEnterSpace?.Invoke(spaceId.text, false);
    }

    private void UseCreatedSpace()
    {
        OnEnterSpace?.Invoke(String.Empty, true);
    }

    private void CreateSpace()
    {
        OnCreateSpace?.Invoke(createSpaceName.text);
    }

    private void SignUp()
    {
        OnSignUp?.Invoke(signUpEmail.text, signUpPassword.text);
    }

    private void SignIn()
    {
        OnSignIn?.Invoke(signInEmail.text, signInPassword.text);
    }

    private void ExitSpace()
    {
        OnExitSpace?.Invoke();
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
        exitSpaceButton.onClick.RemoveListener(ExitSpace);
    }
}