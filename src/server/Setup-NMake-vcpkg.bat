@echo off & setlocal enableextensions enabledelayedexpansion
::
:: Setup prerequisites and build NymphCast Server (MSVC).
::
:: Created 29 January 2022.
:: Copyright (c) 2021 Nyanko.ws
::
:: Usage: Setup-NMake-vcpkg [POCO_ROOT=path/to/lib] [TODO] [target]
::

:: Install vcpkg tool:
:: > git clone https://github.com/microsoft/vcpkg
:: > .\vcpkg\bootstrap-vcpkg.bat -disableMetrics
::

echo.

set INSTALL_PREFIX=D:\Programs\NymphCastServer

set NC_LNKCRT=-MD
:: set NC_LNKCRT=-MT

set NC_CONFIG=Release
:: set NC_CONFIG=Debug

set NCS_TGT_BITS=64
set NCS_TGT_ARCH=x%NCS_TGT_BITS%

set VCPKG_TRIPLET=%NCS_TGT_ARCH%-windows
:: set VCPKG_TRIPLET=%NCS_TGT_ARCH%-windows-static

:: Select static/dynamic linking

:: Check for 64-bit Native Tools Command Prompt

if not [%VSCMD_ARG_TGT_ARCH%] == [%NCS_TGT_ARCH%] (
    echo [Make sure to run these commands in a '%NCS_TGT_BITS%-bit Native Tools Command Prompt'; expecting 'NCS_TGT_ARCH', got '%VSCMD_ARG_TGT_ARCH%'. Bailing out.]
    endlocal & goto :EOF
)

if [%VCPKG_ROOT%] == [] (
    echo [Make sure to environment variable 'VCPKG_ROOT' point to you vcpkg installation; it's empty or does not exist. Bailing out.]
    endlocal & goto :EOF
)

if [%NYMPHRPC_ROOT%] == [] (
    set NYMPHRPC_ROOT=D:\Libraries\NymphRPC
)

if [%LIBNYMPHCAST_ROOT%] == [] (
    set LIBNYMPHCAST_ROOT=D:\Libraries\LibNymphCast
)

:: Make sure NymphRPC and LibNymphCast will be build with the same Poco version:

:: TODO check for proper lib, using NC_LNKCRT_MT (to be added above): mt or no mt

:: Poco[core]:
:: Note: if one subpackage is to be installed, install them all at once.

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco" (
    echo Setup NCS: Poco[core] is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco".
) else (
    echo [Installing vcpkg openssl poco[core,netssl,sqlite3]; please be patient, this may take 30 to 60 minutes...]
    vcpkg install --recurse --triplet %VCPKG_TRIPLET% openssl poco[core,netssl,sqlite3]
)

:: Poco[netssl]:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco\Net\HTTPSClientSession.h" (
    echo Setup NCS: OpenSSL Poco[core,netssl] is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco\Net\HTTPSClientSession.h".
) else (
    echo [Installing vcpkg openssl poco[core,netssl,sqlite3]; please be patient, this may take 30 to 60 minutes...]
    @REM vcpkg install --recurse --triplet %VCPKG_TRIPLET% openssl poco[netssl]
    vcpkg install --recurse --triplet %VCPKG_TRIPLET% openssl poco[core,netssl,sqlite3]
)

:: Poco[sqlite3]:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco\Data\SQLite\Connector.h" (
    echo Setup NCS: OpenSSL Poco[core,netssl,sqlite3] is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco\Data\SQLite\Connector.h".
) else (
    echo [Installing vcpkg openssl poco[core,netssl,sqlite3]; please be patient, this may take 30 to 60 minutes...]
    vcpkg install --recurse --triplet %VCPKG_TRIPLET% openssl poco[core,netssl,sqlite3]
)

set POCO_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: Pthreads:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\pthread.h" (
    echo Setup NCS: Pthreads is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\pthread.h".
) else (
    echo [Installing vcpkg Pthreads; please be patient, this may take about a minute...]
    vcpkg install --triplet %VCPKG_TRIPLET% pthreads
)

::set PTHREADS_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: SDL2:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\SDL2" (
    echo Setup NCS: SDL2 is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\SDL2".
) else (
    echo [Installing vcpkg SDL2[]; please be patient, this may take several minutes...]
    vcpkg install --triplet %VCPKG_TRIPLET% sdl2
)

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\SDL2\SDL_image.h" (
    echo Setup NCS: SDL2-image is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\SDL2\SDL_image.h".
) else (
    echo [Installing vcpkg SDL2-image; please be patient, this may take several minutes...]
    vcpkg install --recurse --triplet %VCPKG_TRIPLET% sdl2-image[libjpeg-turbo,libwebp]
)

set SDL2_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: Ffmpeg:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\libpostproc" (
    echo Setup NCS: Ffmpeg is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\libpostproc".
) else (
    echo [Installing vcpkg Ffmpeg; please be patient, this may take a long time, in the about 30 minutes...]
    vcpkg install --recurse --triplet %VCPKG_TRIPLET% ffmpeg[postproc]
)

set FFMPEG_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%
set LIBAVUTIL_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: FreeImage:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\FreeImage.h" (
    echo Setup NCS: FreeImage is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\FreeImage.h".
) else (
    echo [Installing vcpkg FreeImage; please be patient, this may take about 10 minutes...]
    vcpkg install --triplet %VCPKG_TRIPLET% freeimage
)

set FREEIMAGE_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: FreeType:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\freetype" (
    echo Setup NCS: FreeType is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\freetype".
) else (
    echo [Installing vcpkg FreeType; please be patient, this may take about several minutes...]
    vcpkg install --triplet %VCPKG_TRIPLET% freetype
)

set FREETYPE_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: RapidJSON:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\rapidjson" (
    echo Setup NCS: RapidJSON is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\rapidjson".
) else (
    echo [Installing vcpkg RapidJSON; please be patient, this may take about a minute...]
    vcpkg install --triplet %VCPKG_TRIPLET% rapidjson
)

set RAPIDJSON_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: Curl:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\curl" (
    echo Setup NCS: Curl is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\curl".
) else (
    echo [Installing vcpkg Curl; please be patient, this may take about several minutes...]
    vcpkg install --triplet %VCPKG_TRIPLET% curl
)

set CURL_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: NymphRPC - Download and build NymphRPC dependency:

if exist "%NYMPHRPC_ROOT%\include\nymph\nymph.h" (
    echo Setup NCS: NymphRPC has been installed at "%NYMPHRPC_ROOT%".
) else (
    echo Setup NCS: NymphRPC not found at "%NYMPHRPC_ROOT%".

    git clone --depth 1 https://github.com/MayaPosch/NymphRPC.git

    :: TODO temporary: Setup for NympRPC not yet in repository.
    if exist "D:\Own\Martin" (
        xcopy /y ..\..\..\MartinMoene\NymphCast-Scenarios\code\NMake\NymphRPC\NMakefile NymphRPC
        xcopy /y ..\..\..\MartinMoene\NymphCast-Scenarios\code\NMake\NymphRPC\Setup-NMake-vcpkg.bat NymphRPC
    )

    cd NymphRPC & call Setup-NMake-vcpkg.bat & cd ..

    rmdir /s /q NymphRPC
)

:: LibNymphCast - Download and build LibNymphCast dependency:

if exist "%LIBNYMPHCAST_ROOT%\include\nymphcast_client.h" (
    echo Setup NCS: LibNymphCast has been installed at "%LIBNYMPHCAST_ROOT%".
) else (
    echo Setup NCS: LibNymphCast not found at "%LIBNYMPHCAST_ROOT%".

    git clone --depth 1 https://github.com/MayaPosch/LibNymphCast.git

    :: TODO temporary: Setup for LibNymphCast not yet in repository.
    if exist "D:\Own\Martin" (
        xcopy /y ..\..\..\MartinMoene\NymphCast-Scenarios\code\NMake\LibNymphCast\NMakefile LibNymphCast
        xcopy /y ..\..\..\MartinMoene\NymphCast-Scenarios\code\NMake\LibNymphCast\Setup-NMake-vcpkg.bat LibNymphCast
    )

    cd LibNymphCast & call Setup-NMake-vcpkg.bat & cd ..

    rmdir /s /q LibNymphCast
)

:: Finally, build NymphCast Server:

nmake -nologo -f NMakefile ^
         NC_CONFIG=%NC_CONFIG% ^
         NC_LNKCRT=%NC_LNKCRT% ^
         POCO_ROOT=%POCO_ROOT% ^
         SDL2_ROOT=%SDL2_ROOT% ^
       FFMPEG_ROOT=%FFMPEG_ROOT% ^
    FREEIMAGE_ROOT=%FREEIMAGE_ROOT% ^
     FREETYPE_ROOT=%FREETYPE_ROOT% ^
    LIBAVUTIL_ROOT=%LIBAVUTIL_ROOT% ^
    RAPIDJSON_ROOT=%RAPIDJSON_ROOT% ^
         CURL_ROOT=%CURL_ROOT% ^
     NYMPHRPC_ROOT=%NYMPHRPC_ROOT% ^
 LIBNYMPHCAST_ROOT=%LIBNYMPHCAST_ROOT% ^
    INSTALL_PREFIX=%INSTALL_PREFIX% ^
        %*

echo.

endlocal

:: End of file
