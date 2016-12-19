@echo off
for %%d in (inc lib console gui) do (
    pushd %%d
    for /f %%f in ('dir /b *.cpp *.h') do clang-format.exe -i %%f
    popd
)
