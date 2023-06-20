#Requires -RunAsAdministrator

Set-ExecutionPolicy Bypass -Scope Process -Force

# install chocolatey if not installed
if (!(Test-Path -Path "$env:ProgramData\Chocolatey")) {
  Invoke-Expression((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
}

$Packages = 'git', 'vscode', 'python', 'llvm', 'docker-desktop', 'cmake'

ForEach ($PackageName in $Packages)
{
    choco install $PackageName -y
}
