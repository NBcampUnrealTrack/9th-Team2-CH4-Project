@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul

:: ==============================================================
:: Auto-Generate steam_appid.txt if missing
:: ==============================================================
if not exist "%~dp0steam_appid.txt" (
    echo 480> "%~dp0steam_appid.txt"
    echo [INFO] Created steam_appid.txt in project root.
)

if not exist "%~dp0Binaries\Win64" (
    mkdir "%~dp0Binaries\Win64"
)

if not exist "%~dp0Binaries\Win64\steam_appid.txt" (
    echo 480> "%~dp0Binaries\Win64\steam_appid.txt"
    echo [INFO] Created steam_appid.txt in Binaries\Win64.
)

:: ==============================================================
:: 1. Project and Map Definitions
:: ==============================================================
set "PROJECT_PATH=%~dp0BARUGame.uproject"
set "MENU_MAP=/Game/BARUGame/Maps/MainMenuLevel"
set "LOBBY_MAP=/Game/BARUGame/Maps/MainLobbyLevel"
set "TEST_MAP=/Game/BARUGame/Maps/TestGym"

:: Verify project descriptor existence
if not exist "%PROJECT_PATH%" (
    echo [ERROR] Project file not found:
    echo %PROJECT_PATH%
    echo Please place this launcher inside the root project directory.
    pause
    exit /b
)

:: ==============================================================
:: 2. Unreal Engine 5.8 Path Auto-Detection
:: ==============================================================
set "UE_EDITOR="

:: Search Registry for Epic Games Installation
for /f "tokens=2* skip=2" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v "InstalledDirectory" 2^>nul') do (
    if exist "%%B\Engine\Binaries\Win64\UnrealEditor.exe" (
        set "UE_EDITOR=%%B\Engine\Binaries\Win64\UnrealEditor.exe"
        goto ENGINE_RESOLVED
    )
)

:: Search Standard Drive Paths
set "CANDIDATE_PATHS[0]=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "CANDIDATE_PATHS[1]=D:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "CANDIDATE_PATHS[2]=E:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "CANDIDATE_PATHS[3]=C:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "CANDIDATE_PATHS[4]=D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "CANDIDATE_PATHS[5]=E:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "CANDIDATE_PATHS[6]=F:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"

for /L %%i in (0,1,6) do (
    if exist "!CANDIDATE_PATHS[%%i]!" (
        set "UE_EDITOR=!CANDIDATE_PATHS[%%i]!"
        goto ENGINE_RESOLVED
    )
)

:: Fallback: Manual User Input
:PROMPT_MANUAL_PATH
echo ================================================================
echo [WARNING] Unreal Engine 5.8 executable was not found automatically.
echo ================================================================
set /p USER_INPUT_PATH="Enter the full path to UnrealEditor.exe: "
set "USER_INPUT_PATH=%USER_INPUT_PATH:"=%"

if exist "%USER_INPUT_PATH%" (
    set "UE_EDITOR=%USER_INPUT_PATH%"
    goto ENGINE_RESOLVED
) else (
    echo [ERROR] Specified path does not exist. Please re-enter.
    pause
    goto PROMPT_MANUAL_PATH
)

:ENGINE_RESOLVED
echo [INFO] Detected Engine: "%UE_EDITOR%"

:: ==============================================================
:: 3. Launcher Selection Menu
:: ==============================================================
:MAIN_MENU
cls
echo ================================================================
echo          BARU Game Multiplayer / Steam Test Launcher
echo ================================================================
echo  Engine  : %UE_EDITOR%
echo  Project : %PROJECT_PATH%
echo ================================================================
echo  [1] [Steam] Launch Game (MainMenuLevel - Full Session/UI Flow)
echo  [2] [Steam] Direct Lobby Host (MainLobbyLevel as Listen Server)
echo  [3] [Local] 2-Player Split-Screen Test (Host + Client 127.0.0.1)
echo  [4] [Direct] Direct TestGym Host (Direct Combat and AI Testing)
echo  [5] Exit
echo ================================================================
set /p MENU_CHOICE="Enter selection [1-5]: "

if "%MENU_CHOICE%"=="1" goto LAUNCH_STEAM_MAIN
if "%MENU_CHOICE%"=="2" goto LAUNCH_STEAM_LOBBY
if "%MENU_CHOICE%"=="3" goto LAUNCH_LOCAL_DUAL
if "%MENU_CHOICE%"=="4" goto LAUNCH_DIRECT_GYM
if "%MENU_CHOICE%"=="5" goto TERMINATE_SCRIPT
goto MAIN_MENU

:: --- [1] Full Game Flow via MainMenuLevel ---
:LAUNCH_STEAM_MAIN
echo [STATUS] Launching Game at MainMenuLevel (Steam Enabled)...
start "" "%UE_EDITOR%" "%PROJECT_PATH%" %MENU_MAP% -game -log -WINDOWED ResX=1280 ResY=720 -WinX=100 -WinY=100
goto TERMINATE_SCRIPT

:: --- [2] Steam Direct Lobby Listen Server ---
:LAUNCH_STEAM_LOBBY
echo [STATUS] Launching Steam Listen Server directly at MainLobbyLevel...
start "" "%UE_EDITOR%" "%PROJECT_PATH%" %LOBBY_MAP%?listen -game -log -WINDOWED ResX=1280 ResY=720 -WinX=30 -WinY=50
goto TERMINATE_SCRIPT

:: --- [3] Local IP Direct 2-Player Split Test ---
:LAUNCH_LOCAL_DUAL
echo [STATUS] Launching Local Listen Server Host at MainLobbyLevel...
start "Host (Server)" "%UE_EDITOR%" "%PROJECT_PATH%" %LOBBY_MAP%?listen -game -log -WINDOWED ResX=960 ResY=540 -WinX=30 -WinY=50
timeout /t 4 /nobreak >nul
echo [STATUS] Launching Local Client (Connecting to 127.0.0.1)...
start "Client 1" "%UE_EDITOR%" "%PROJECT_PATH%" 127.0.0.1 -game -log -WINDOWED ResX=960 ResY=540 -WinX=1000 -WinY=50
goto TERMINATE_SCRIPT

:: --- [4] Direct Combat and AI Sandbox Host ---
:LAUNCH_DIRECT_GYM
echo [STATUS] Launching Direct TestGym Listen Server...
start "" "%UE_EDITOR%" "%PROJECT_PATH%" %TEST_MAP%?listen -game -log -WINDOWED ResX=1280 ResY=720 -WinX=50 -WinY=50
goto TERMINATE_SCRIPT

:TERMINATE_SCRIPT
exit /b