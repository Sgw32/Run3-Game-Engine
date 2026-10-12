@echo off
REM Luniverse streamed-world evaluation. Extra arguments override defaults.

"%~dp0build\install\windows-release\bin\run3_shell.exe" ^
    --renderer d3d11 ^
    --content-root "C:\Run3-Game-Engine\Games\The Long Way\TheLongWay" ^
    --content-variant luniverse ^
    --content-overlay "%~dp0derived-content\luniverse" ^
    --map luniverse ^
    --user-dir ".\build\user\windows-release" ^
    --fullscreen ^
    --lighting-pipeline legacy-forward --shadow-quality high --exposure 1.6 ^
    --resolution 1920x1080 --fov 75 ^
    --texture-quality high --model-quality high --scene-quality high ^
    --audio-backend miniaudio ^
    %*

pause
