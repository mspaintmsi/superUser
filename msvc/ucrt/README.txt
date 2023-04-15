Visual Studio / UCRT
====================

By default, Visual Studio (since 2015) uses the Visual C++ runtime (vcruntimeXXX.dll) included in the latest "Visual C++ Redistributable", and the UCRT dlls. The compiled program needs these DLLs to run.

To eliminate the dependency on the redistributable part, while not increasing the size of the executable too much, it is possible to statically link with the VC++ runtime (smallest), and to dynamically link against the UCRT (biggest).

The generated executables require the UCRT dlls to run (included in Windows 10 or installed by the cumulative updates in older versions).

The executable size is around 23-29 KB.

- Open the "msvc\ucrt\superUser" project with Visual Studio.
- In the toolbar, choose the "Release" configuration.
- Select the platform (x64 or x86).
- Build the project (menu Build > Build Solution, or press Ctrl+Shift+B).

This creates superUser32.exe or superUser64.exe in the project directory ("msvc\ucrt").
