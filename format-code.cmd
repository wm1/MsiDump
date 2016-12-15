@echo off
for %%d in (. console gui) do (
    pushd %%d
    for /f %%f in ('dir /b *.cpp *.h') do "%USERPROFILE%/.vscode/extensions/ms-vscode.cpptools-0.9.3/bin/../LLVM/bin/clang-format.exe" -i %%f
    popd
)
