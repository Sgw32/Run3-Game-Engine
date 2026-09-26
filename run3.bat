@echo off
REM cd /d C:\Run3-Game-Engine

build\install\windows-debug\bin\run3_shell.exe ^
    --renderer d3d11 ^
    --content-root "C:\Run3-Game-Engine\Games\The Long Way\TheLongWay" ^
    --user-dir ".\build\user\windows-debug" ^
    --fullscreen ^
    --resolution 1920x1080 --fov 75 ^
    --texture-quality high --model-quality high --scene-quality high ^
    --audio-backend miniaudio

pause