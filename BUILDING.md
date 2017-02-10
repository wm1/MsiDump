# Building MsiDump

## Prerequisites:

    - https://www.visualstudio.com/downloads/
    - https://github.com/Microsoft/vcpkg
    - https://cmake.org/

## Steps

    - git clone https://github.com/Microsoft/vcpkg
    - vcpkg install wtl:x86-windows
    - vcpkg install wtl:x64-windows

    - md build & cd build
    - cmake -D CMAKE_TOOLCHAIN_FILE=[your-vcpkg-root]\scripts\buildsystems\vcpkg.cmake
            -G "Visual Studio 14 2015 Win64"
            ..
    - msbuild

## Test

    "cd test" and run the just-built "MsiDumpTest.exe". If everything works it should output:
```
Extract data\test.msi to data\out
Compare data\in with data\out
Test passes
```