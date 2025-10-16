# screenSh0ter

This project is for capturing a screenshot of Chrome or Firefox then sending the screenshot to a server.


## Description

This project is the client for screenSh0ter and will take a screen shot of either Chrome or Firefox and save it to the root of the users profile, 
It will then read the bytes of the screen shot and then convert to base64 and send it to the reciver of your choosing. Now I cant code worth ****
and everyline here took me and AI 30 minutes for each line because I am not the sharpest tool in the shed, so dont expect the greatest.

## Getting Started

### Dependencies

- Windows SDK (UIAutomation, GDI32)
- libcurl (HTTP uploads)
- ATL (COM smart pointers)

```cmd
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install curl:x64-windows
.\vcpkg integrate install  # Integrates with Visual Studio
```
#### Visual Studio Project Configuration
Project Properties (x64 Configuration):
VC++ Directories:

Include Directories: C:\vcpkg\installed\x64-windows\include
Library Directories: C:\vcpkg\installed\x64-windows\lib

#### C/C++ -> Preprocessor:
- Processor Definitions

```text
_CRT_SECURE_NO_WARNINGS
CURL_STATICLIB
WIN32_LEAN_AND_MEAN
```

Linker -> Input -> Additional Dependencies

```text
libcurl.lib
ole32.lib
oleaut32.lib
user32.lib
gdi32.lib
ws2_32.lib
wldap32.lib
crypt32.lib
advapi32.lib
normaliz.lib
```

### Installing

git clone it and you will have to compile it. There are some options in the code that you will have to set before you compile labled with (CHANGE THIS).  

### Executing program

```bash
python3 customReciever.py 
```

## Help

Common Fixes:

1. "curl/curl.h not found": vcpkg include path missing
2. CURL symbols undefined: Link libcurl.lib + dependencies
3. UIAutomation errors: Windows SDK + ole32.lib
4. Black screenshots: Run as Administrator + PrintWindow(PW_CLIENTONLY)

## Authors

- TheDyingYak https://github.com/TheDyingYak


## Version History

* 0.1
    * Initial Release

## License

This project is licensed under the [The Unlicense] License - see the LICENSE.md file for details

## Acknowledgments

