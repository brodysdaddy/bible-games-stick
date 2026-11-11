@echo off
REM Compile and upload the Bible Games firmware to an M5StickS3.
REM Usage:  upload.bat [COM port]        (default: COM4)
REM If the upload can't connect: hold the side button ~2 s until the green
REM LED blinks (download mode), then run this again.
setlocal
set PORT=%~1
if "%PORT%"=="" set PORT=COM4
set FQBN=m5stack:esp32:m5stack_sticks3
set ROOT=%~dp0

arduino-cli compile --fqbn %FQBN% --library "%ROOT%lib\TriviaCore" --library "%ROOT%lib\TowerOfBabel" "%ROOT%bible_games"
if errorlevel 1 exit /b 1
arduino-cli upload -p %PORT% --fqbn %FQBN% "%ROOT%bible_games"
