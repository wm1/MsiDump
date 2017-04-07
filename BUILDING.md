# Building MsiDump

## Prerequisites:

    - https://www.visualstudio.com/downloads/
    - https://github.com/Microsoft/vcpkg
    - https://cmake.org/

## Steps

    - vcpkg integrate install
    - vcpkg install wtl:x64-windows
    - md build & cd build
    - cmake -G "Visual Studio 15 2017" -A x64 -T host=x64 ..
    - msbuild MsiDump.sln

## Test

    "cd test" and run the just-built "MsiDumpTest.exe". If everything works it should output:
```
Extract data\test.msi to data\out
Compare data\in with data\out
Test passes
```