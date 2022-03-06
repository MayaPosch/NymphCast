@echo off & setlocal enableextensions enabledelayedexpansion
::
:: Setup prerequisites and build NymphCast GUI Player (MSVC).
::
:: Created 2 February 2022.
:: Copyright (c) 2021 Nyanko.ws
::
:: Usage: Setup-NMake-vcpkg [LIBNYMPHCAST_ROOT=path/to/lib] [NYMPHRPC_ROOT=path/to/lib] [POCO_ROOT=path/to/lib] [QT5_ROOT=path/to/lib] [target]
::

:: Install vcpkg tool:
:: > git clone https://github.com/microsoft/vcpkg
:: > .\vcpkg\bootstrap-vcpkg.bat -disableMetrics
:: > set VCPKG_ROOT=/path/to/vcpkg-folder
::

echo.

set INSTALL_PREFIX=D:\Programs\NymphCastPlayer

:: Note: static building does not yet work.
set NC_STATIC=0
:: set NC_STATIC=1

set NC_CONFIG=Release
:: set NC_CONFIG=Debug

set NC_CONSOLE=NOCONSOLE
:: set NC_CONSOLE=CONSOLE

set NC_TGT_BITS=64
set NC_TGT_ARCH=x%NC_TGT_BITS%

set NC_LNKCRT=-MD
set VCPKG_TRIPLET=x64-windows

if [%NC_STATIC%] == [1] (
    set NC_LNKCRT=-MT
    set VCPKG_TRIPLET=x64-windows-static
    echo [Setup NCP: static build does not yet work. Continuing.]
)

:: Check for 64-bit Native Tools Command Prompt

if not [%VSCMD_ARG_TGT_ARCH%] == [%NC_TGT_ARCH%] (
    echo [Setup NCP: Make sure to run these commands in a '64-bit Native Tools Command Prompt'; expecting 'x64', got '%VSCMD_ARG_TGT_ARCH%'. Bailing out.]
    endlocal & goto :EOF
)

:: Check for vcpkg:

set vcpkg=%VCPKG_ROOT%\vcpkg.exe

if [%VCPKG_ROOT%] == [] (
    echo [Setup NCP: Make sure environment variable 'VCPKG_ROOT' points to your vcpkg installation; it's empty or does not exist. Bailing out.]
    endlocal & goto :EOF
)

:: NymphRPC and LibNymphCast libraries:

if [%NYMPHRPC_ROOT%] == [] (
    set NYMPHRPC_ROOT=D:\Libraries\NymphRPC
)

if [%LIBNYMPHCAST_ROOT%] == [] (
    set LIBNYMPHCAST_ROOT=D:\Libraries\LibNymphCast
)

:: Make sure NymphRPC and LibNymphCast will be build with the same Poco version:

:: TODO check for proper lib, using NC_LNKCRT_MT (to be added above): mt or no mt

:: Poco[core]:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco" (
    echo Setup NCP: Poco is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco".
) else (
    echo Installing vcpkg Poco; please be patient, this may take about 10 minutes...
    %vcpkg% install --triplet %VCPKG_TRIPLET% poco
)

echo Setup NCP: Using POCO_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

set POCO_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: Qt5[core]:
:: Note temporary workaround as vcpkg did not succeed with Qt5:

if not [%QT5_ROOT%] == [] (
    echo Setup NCP: Qt is already installed at "%QT5_ROOT%".
    set QT5_ROOT=%QT5_ROOT%
    set QT5_INCLUDE_FIX=
) else (
    if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Qt5" (
        echo Setup NCP: Qt is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Qt5".
    ) else (
        echo Installing vcpkg Qt5; please be patient, this may take about 2 hours...
        %vcpkg% install --triplet %VCPKG_TRIPLET% qt5
    )

    echo Setup NCP: Using QT5_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

    set QT5_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%
    set QT5_INCLUDE_FIX=qt5/
)

:: NymphRPC - Download and build NymphRPC dependency:

if exist "%NYMPHRPC_ROOT%\include\nymph\nymph.h" (
    echo Setup NCP: NymphRPC has been installed at "%NYMPHRPC_ROOT%".
) else (
    echo Setup NCP: NymphRPC not found at "%NYMPHRPC_ROOT%".

    git clone --depth 1 https://github.com/MayaPosch/NymphRPC.git

    cd NymphRPC & call Setup-NMake-vcpkg.bat & cd ..

    rmdir /s /q NymphRPC
)

:: LibNymphCast - Download and build LibNymphCast dependency:

if exist "%LIBNYMPHCAST_ROOT%\include\nymphcast_client.h" (
    echo Setup NCP: LibNymphCast has been installed at "%LIBNYMPHCAST_ROOT%".
) else (
    echo Setup NCP: LibNymphCast not found at "%LIBNYMPHCAST_ROOT%".

    git clone --depth 1 https://github.com/MayaPosch/LibNymphCast.git

    cd LibNymphCast & call Setup-NMake-vcpkg.bat & cd ..

    rmdir /s /q LibNymphCast
)

:: Finally, build NymphCast Media Server:

nmake -nologo -f NMakefile ^
         NC_STATIC=%NC_STATIC% ^
         NC_CONFIG=%NC_CONFIG% ^
        NC_CONSOLE=%NC_CONSOLE% ^
         NC_LNKCRT=%NC_LNKCRT% ^
          QT5_ROOT=%QT5_ROOT% ^
   QT5_INCLUDE_FIX=%QT5_INCLUDE_FIX% ^
         POCO_ROOT=%POCO_ROOT% ^
     NYMPHRPC_ROOT=%NYMPHRPC_ROOT% ^
 LIBNYMPHCAST_ROOT=%LIBNYMPHCAST_ROOT% ^
    INSTALL_PREFIX=%INSTALL_PREFIX% ^
        %*

echo.

endlocal

:: End of file
