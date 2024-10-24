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
:: > set VCPKG_ROOT=/path/to/vcpkg-folder
::

echo.

set INSTALL_PREFIX=D:\Programs\NymphCastServer

:: Note: static building does not yet work.
set NC_STATIC=0
:: set NC_STATIC=1

set NC_CONFIG=Release
:: set NC_CONFIG=Debug

set NC_TGT_BITS=64
set NC_TGT_ARCH=x%NC_TGT_BITS%

set NC_LNKCRT=-MD
set VCPKG_TRIPLET=x64-windows

if [%NC_STATIC%] == [1] (
    set NC_LNKCRT=-MT
    set VCPKG_TRIPLET=x64-windows-static
    echo [Setup NCS: static build does not yet work. Continuing.]
)

:: Determine Visual Studio year:

:: VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\

set VS_YEAR=%VSINSTALLDIR%
set VS_YEAR=%VS_YEAR:C:=%
set VS_YEAR=%VS_YEAR:D:=%
set VS_YEAR=%VS_YEAR:E:=%
set VS_YEAR=%VS_YEAR:\Community\=%
set VS_YEAR=%VS_YEAR:\Preview\=%
set VS_YEAR=%VS_YEAR:\Programs\=%
set VS_YEAR=%VS_YEAR:\Program Files\=%
set VS_YEAR=%VS_YEAR:\Program Files (x86)\=%
set VS_YEAR=%VS_YEAR:Microsoft Visual Studio\=%

if not [%VS_YEAR%] == [2017] (
if not [%VS_YEAR%] == [2019] (
if not [%VS_YEAR%] == [2022] (
    echo [Setup NCS: Expecting Visual Studio year '2017', 2019, or '2022'; got '%VS_YEAR%'. Bailing out.]
    endlocal & goto :EOF
)))

:: Check for 64-bit Native Tools Command Prompt

if not [%VSCMD_ARG_TGT_ARCH%] == [%NC_TGT_ARCH%] (
    echo [Setup NCS: Make sure to run these commands in a '%NC_TGT_BITS%-bit Native Tools Command Prompt'; expecting 'NC_TGT_ARCH', got '%VSCMD_ARG_TGT_ARCH%'. Bailing out.]
    endlocal & goto :EOF
)

:: Check for vcpkg:

set vcpkg="%VCPKG_ROOT%\vcpkg.exe"

if ["%VCPKG_ROOT%"] == [] (
    echo [Setup NCS: Make sure environment variable 'VCPKG_ROOT' points to your vcpkg installation; it's empty or does not exist. Bailing out.]
    endlocal & goto :EOF
)

:: NymphRPC and LibNymphCast libraries:

if ["%NYMPHRPC_ROOT%"] == [] (
    set NYMPHRPC_ROOT=D:\Libraries\NymphRPC
)

if ["%LIBNYMPHCAST_ROOT%"] == [] (
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
    "%vcpkg%" install --recurse --triplet %VCPKG_TRIPLET% openssl poco[core,netssl,sqlite3]
)

:: Poco[netssl]:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco\Net\HTTPSClientSession.h" (
    echo Setup NCS: OpenSSL Poco[core,netssl] is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco\Net\HTTPSClientSession.h".
) else (
    echo [Installing vcpkg openssl poco[core,netssl,sqlite3]; please be patient, this may take 30 to 60 minutes...]
    @REM %vcpkg% install --recurse --triplet %VCPKG_TRIPLET% openssl poco[netssl]
    "%vcpkg%" install --recurse --triplet %VCPKG_TRIPLET% openssl poco[core,netssl,sqlite3]
)

:: Poco[sqlite3]:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco\Data\SQLite\Connector.h" (
    echo Setup NCS: OpenSSL Poco[core,netssl,sqlite3] is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\Poco\Data\SQLite\Connector.h".
) else (
    echo [Installing vcpkg openssl poco[core,netssl,sqlite3]; please be patient, this may take 30 to 60 minutes...]
    "%vcpkg%" install --recurse --triplet %VCPKG_TRIPLET% openssl poco[core,netssl,sqlite3]
)

set POCO_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: Pthreads:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\pthread.h" (
    echo Setup NCS: Pthreads is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\pthread.h".
) else (
    echo [Installing vcpkg Pthreads; please be patient, this may take about a minute...]
    "%vcpkg%" install --triplet %VCPKG_TRIPLET% pthreads
)

::set PTHREADS_ROOT=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%

:: SDL2:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\SDL2" (
    echo Setup NCS: SDL2 is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\SDL2".
) else (
    echo [Installing vcpkg SDL2[]; please be patient, this may take several minutes...]
    "%vcpkg%" install --triplet %VCPKG_TRIPLET% sdl2
)

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\SDL2\SDL_image.h" (
    echo Setup NCS: SDL2-image is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\SDL2\SDL_image.h".
) else (
    echo [Installing vcpkg SDL2-image; please be patient, this may take several minutes...]
    "%vcpkg%" install --recurse --triplet %VCPKG_TRIPLET% sdl2-image[libjpeg-turbo,libwebp]
)

set SDL2_ROOT="%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%"

:: Ffmpeg:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\libpostproc" (
    echo Setup NCS: Ffmpeg is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\libpostproc".
) else (
    echo [Installing vcpkg Ffmpeg; please be patient, this may take a long time, in the about 30 minutes...]
    "%vcpkg%" install --recurse --triplet %VCPKG_TRIPLET% ffmpeg[postproc]
)

set FFMPEG_ROOT="%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%"
set LIBAVUTIL_ROOT="%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%"

:: FreeImage:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\FreeImage.h" (
    echo Setup NCS: FreeImage is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\FreeImage.h".
) else (
    echo [Installing vcpkg FreeImage; please be patient, this may take about 10 minutes...]
    "%vcpkg%" install --triplet %VCPKG_TRIPLET% freeimage
)

set FREEIMAGE_ROOT="%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%"

:: FreeType:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\freetype" (
    echo Setup NCS: FreeType is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\freetype".
) else (
    echo [Installing vcpkg FreeType; please be patient, this may take about several minutes...]
    "%vcpkg%" install --triplet %VCPKG_TRIPLET% freetype
)

set FREETYPE_ROOT="%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%"

:: RapidJSON:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\rapidjson" (
    echo Setup NCS: RapidJSON is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\rapidjson".
) else (
    echo [Installing vcpkg RapidJSON; please be patient, this may take about a minute...]
    "%vcpkg%" install --triplet %VCPKG_TRIPLET% rapidjson
)

set RAPIDJSON_ROOT="%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%"

:: Curl:

if exist "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\curl" (
    echo Setup NCS: Curl is already installed at "%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%\include\curl".
) else (
    echo [Installing vcpkg Curl; please be patient, this may take about several minutes...]
    "%vcpkg%" install --triplet %VCPKG_TRIPLET% curl
)

set CURL_ROOT="%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%"

:: NymphRPC - Download and build NymphRPC dependency:

if exist "%NYMPHRPC_ROOT%\include\nymph\nymph.h" (
    echo Setup NCS: NymphRPC has been installed at "%NYMPHRPC_ROOT%".
) else (
    echo Setup NCS: NymphRPC not found at "%NYMPHRPC_ROOT%".

    git clone --depth 1 https://github.com/MayaPosch/NymphRPC.git

    cd NymphRPC & call Setup-NMake-vcpkg.bat & cd ..

    rmdir /s /q NymphRPC
)

:: LibNymphCast - Download and build LibNymphCast dependency:

if exist "%LIBNYMPHCAST_ROOT%\include\nymphcast_client.h" (
    echo Setup NCS: LibNymphCast has been installed at "%LIBNYMPHCAST_ROOT%".
) else (
    echo Setup NCS: LibNymphCast not found at "%LIBNYMPHCAST_ROOT%".

    git clone --depth 1 https://github.com/MayaPosch/LibNymphCast.git

    cd LibNymphCast & call Setup-NMake-vcpkg.bat & cd ..

    rmdir /s /q LibNymphCast
)

:: Finally, build NymphCast Server:

nmake -nologo -f NMakefile ^
         NC_STATIC=%NC_STATIC% ^
         NC_CONFIG=%NC_CONFIG% ^
         NC_LNKCRT=%NC_LNKCRT% ^
           VS_YEAR=%VS_YEAR% ^
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
