# srvman
Windows Service Manager is a small tool that simplifies all common tasks related to Windows services. It can create services (both Win32 and Legacy Driver) without restarting Windows, delete existing services and change service configuration. It has both GUI and Command-line modes.

# How to build
- You need at least MSVS2019 and up
- set env var named `WTLDIR` which points to `Include` dir inside WTL10 folder, eg:
```
c:\Dev\libraries\WTL\Include
```
- Open up bzscore.sln, select Debug (Win32) Platform/x64 and build the lib
- Open up srvman.sln, select Debug Platform/x64 and build it as well
