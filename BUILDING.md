# Building MsiDump

The code can be built in several different ways. The plan is to only keep the CMake option.

    Option 1. Use CMake to re-create VC++ project files and then run MSBuild
    Option 2. Use the existing VC++ project files and then run MSBuild
    Option 3. Use build.exe from WDK version 7.1 or earlier

## Prerequisites:

    - https://www.visualstudio.com/downloads/
    - https://github.com/Microsoft/vcpkg

    Optional:

    - https://cmake.org/
    - https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit

## Steps

    - git clone https://github.com/Microsoft/vcpkg
    - vcpkg install wtl:x86-windows
    - vcpkg install wtl:x64-windows

    Option 1:
    - md build & cd build
    - cmake -D CMAKE_TOOLCHAIN_FILE=D:\git\github\vcpkg\scripts\buildsystems\vcpkg.cmake
            -G "Visual Studio 14 2015 Win64"
            ..
    - msbuild

    Option 2:
    - vcpkg integrate install
    - msbuild

    Option 3:
    - build -cZ

## Test

    "cd test" and run the just-built "MsiDumpTest.exe"
