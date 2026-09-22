@echo off
REM cd /d C:\Run3-Game-Engine

build\install\windows-debug\bin\run3_shell.exe ^
    --renderer d3d11 ^
    --content-root "C:\Run3-Game-Engine\Games\The Long Way\TheLongWay" ^
    --map tlwcao ^
    --user-dir ".\build\user\windows-debug" ^
    --fullscreen ^
    --audio-backend miniaudio

pause