# Test Account Credentials

## File Location

If you want to run tests locally, you will need to create a `test_account_creds.txt` file in the following locations:

C++
- connected-spaces-platform\Tests\Binaries\x64\DebugDLL

WASM
 - connected-spaces-platform\Tests\Web\html\assets

## File Format

`test_account_creds.txt` should contain the following information:

``` 
<main test account email> <main test account password> 
<alt test account email> <alt test account password> 
<oko.tests account email> <oko.tests account password>
```

> **Accounts created for the OKO_TESTS tenant **do not** have to be verified upon account creation).

## Test Account Requirements

* In each instance, the main and alt test accounts must be different accounts. (Ensuring that both are registered and authenticated by the email you are sent post-account-creation. This applies to alias accounts also.)