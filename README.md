## Name

MsiDump, MsiDumpCab - expand Windows Installer Package (.msi) files

## Synopsis
```
    MsiDumpCab [msiFile]

    MsiDump command [options] msiFile [path_to_extract]

    commands (use one of them):
        -list                 list msiFile
        -extract              extract files
        -help                 this help secreen

    options for -list:
        -format:nfspvl        list num, file, size, path, version, lang (DEFAULT:nfsp)

    options for -extract:
        -full_path:yes|no     extract files with full path (DEFAULT:yes)

    Legacy Usage:
        MsiDump msiFile                 - list
        MsiDump msiFile extractPath     - extract
        MsiDump msiFile extractPath -f  - extract -full_path:no,
```

## Description

MsiDump is the command line version of the tool;

while MsiDumpCab provides a WinZip-like user interface.
