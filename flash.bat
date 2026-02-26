@echo off
title Flash Conveyor Firmware
echo ========================================
echo   Flash Firmware - The Conveyor
echo ========================================
echo.

:: Cherche le fichier .bin dans le meme dossier
set BIN_FILE=
for %%f in ("%~dp0conveyor_*.bin") do set BIN_FILE=%%f

if "%BIN_FILE%"=="" (
    echo ERREUR : Aucun fichier conveyor_*.bin trouve.
    echo Placez ce script dans le meme dossier que le fichier .bin
    pause
    exit /b 1
)

echo Fichier detecte : %BIN_FILE%
echo.

:: Verifie si Python est installe
python --version >nul 2>&1
if errorlevel 1 (
    echo Python n'est pas installe.
    echo Telechargement en cours...
    echo Ouvrez https://www.python.org/downloads/ et installez Python.
    start https://www.python.org/downloads/
    pause
    exit /b 1
)

:: Installe esptool si absent
esptool.py version >nul 2>&1
if errorlevel 1 (
    echo Installation de esptool...
    pip install esptool
)

echo.
echo Branchez l'ESP32 en USB puis appuyez sur une touche...
pause >nul

:: Detecte le port COM automatiquement
echo Detection du port...
for /f "tokens=2 delims==" %%a in ('wmic path Win32_SerialPort where "Description like '%%CP210%%' or Description like '%%CH340%%' or Description like '%%FTDI%%'" get DeviceID /value 2^>nul') do set COM_PORT=%%a

if "%COM_PORT%"=="" (
    echo Port non detecte automatiquement.
    set /p COM_PORT="Entrez le port manuellement (ex: COM11) : "
)

echo Port utilise : %COM_PORT%
echo.
echo Flashage en cours... Ne debranchez pas l'ESP32 !
echo.

esptool.py --port %COM_PORT% --baud 921600 write_flash 0x1000 "%BIN_FILE%"

if errorlevel 1 (
    echo.
    echo ERREUR lors du flashage.
    echo Verifiez que l'ESP32 est bien branche et reessayez.
) else (
    echo.
    echo ========================================
    echo   Firmware installe avec succes !
    echo   Vous pouvez debrancher l'ESP32.
    echo ========================================
)

pause
