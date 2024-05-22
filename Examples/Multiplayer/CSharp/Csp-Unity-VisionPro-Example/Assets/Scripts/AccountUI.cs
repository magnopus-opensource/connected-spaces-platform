// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using System;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class AccountUI : MonoBehaviour
{
    public event Action<string, string> SignUpButtonClicked;
    public event Action<string, string> SignInButtonClicked;
    public event Action<string> EnterSpaceButtonClicked;
    public event Action<string> CreateSpaceButtonClicked;
    public event Action DeleteSpaceButtonClicked;
    public event Action LogoutButtonClicked;

    [Header("Sign up and Sign In")]
    [SerializeField] private GameObject signInRootObject;
    [SerializeField] private GameObject signUpRootObject;
    [SerializeField] private GameObject postLoginRootObject;
    [SerializeField] private TMP_InputField signUpEmail;
    [SerializeField] private TMP_InputField signUpPassword;
    [SerializeField] private TMP_InputField signInEmail;
    [SerializeField] private TMP_InputField signInPassword;
    [SerializeField] private Button signUpButton;
    [SerializeField] private Button signInButton;
    [SerializeField] private TextMeshProUGUI signUpMessage;
    [SerializeField] private TextMeshProUGUI signInMessage;

    [Header("Create Space")]
    [SerializeField] private TMP_InputField createSpaceName;
    [SerializeField] private Button createSpaceButton;
    [SerializeField] private Button deleteSpaceButton;

    [Header("Enter Space")]
    [SerializeField] private TMP_InputField spaceId;
    [SerializeField] private Button enterSpaceButton;

    [SerializeField] private Button logoutButton;

    public void SignInSucceeded()
    {
        ResetSignUpSignInUI();
        signInRootObject.SetActive(false);
        postLoginRootObject.SetActive(true);
    }

    public void SignInFailed(string message)
    {
        signInMessage.text = "Sign in error " + message;
    }

    public void CreateSpaceSucceeded(string createdSpaceId)
    {
        spaceId.text = createdSpaceId;
        deleteSpaceButton.gameObject.SetActive(true);
    }

    private void Awake()
    {
        signUpButton.onClick.AddListener(SignUp);
        signInButton.onClick.AddListener(SignIn);
        createSpaceButton.onClick.AddListener(CreateSpace);
        enterSpaceButton.onClick.AddListener(EnterSpace);
        deleteSpaceButton.onClick.AddListener(DeleteSpace);
        logoutButton.onClick.AddListener(Logout);
    }

    private void EnterSpace()
    {
        EnterSpaceButtonClicked?.Invoke(spaceId.text);
    }

    private void CreateSpace()
    {
        CreateSpaceButtonClicked?.Invoke(createSpaceName.text);
    }

    private void SignUp()
    {
        SignUpButtonClicked?.Invoke(signUpEmail.text, signUpPassword.text);
        ResetSignUpSignInUI();
    }

    private void SignIn()
    {
        SignInButtonClicked?.Invoke(signInEmail.text, signInPassword.text);
    }

    private void Logout()
    {
        createSpaceName.text = string.Empty;
        spaceId.text = string.Empty;
        deleteSpaceButton.gameObject.SetActive(false);

        LogoutButtonClicked?.Invoke();
    }

    private void DeleteSpace()
    {
        createSpaceName.text = string.Empty;
        spaceId.text = string.Empty;
        deleteSpaceButton.gameObject.SetActive(false);

        DeleteSpaceButtonClicked?.Invoke();
    }

    private void ResetSignUpSignInUI()
    {
        signUpEmail.text = string.Empty;
        signUpPassword.text = string.Empty;
        signInEmail.text = string.Empty;
        signInPassword.text = string.Empty;
        signInMessage.text = string.Empty;
    }

    private void OnDestroy()
    {
        signUpButton.onClick.RemoveListener(SignUp);
        signInButton.onClick.RemoveListener(SignIn);
        createSpaceButton.onClick.RemoveListener(CreateSpace);
        enterSpaceButton.onClick.RemoveListener(EnterSpace);
        deleteSpaceButton.onClick.RemoveListener(DeleteSpace);
        logoutButton.onClick.RemoveListener(Logout);
    }
}