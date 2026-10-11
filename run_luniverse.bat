@echo off
REM Luniverse streamed-world evaluation. Extra arguments override defaults.

"%~dp0build\install\windows-release\bin\run3_shell.exe" ^
    --renderer d3d11 ^
    --content-root "C:\Run3-Game-Engine\Games\The Long Way\TheLongWay" ^
    --content-variant luniverse ^
    --content-overlay "%~dp0derived-content\luniverse" ^
    --map luniverse ^
    --user-dir ".\build\user\windows-release" ^
    --windowed ^
    --lighting-pipeline fast-forward --shadow-quality low --exposure 0.8 ^
    --resolution 1280x720 --fov 75 ^
    --texture-quality high --model-quality high --scene-quality high ^
    --audio-backend miniaudio ^
    %*

pause
