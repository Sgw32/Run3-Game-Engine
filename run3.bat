@echo off
REM Step 9B preview: extra arguments override the defaults below.
REM run3.bat --lighting-pipeline legacy-forward --shadow-quality low
REM run3.bat --lighting-pipeline fast-forward --shadow-quality off
REM run3.bat --lighting-pipeline pbr --shadow-quality high --exposure 1
REM run3.bat --lighting-pipeline deferred --shadow-quality low
REM These new pipelines are experimental; campaign validation is still in progress.
REM Content overlay is independent of the lighting pipeline. Use an existing hashed copy:
REM run3.bat --lighting-pipeline pbr --content-variant nextgen --content-overlay "%~dp0derived-content\nextgen-v0"
REM nextgen-v0 is currently a byte-identical staging copy, NOT a lighting retune.
REM Use --content-variant original without --content-overlay for original content.
REM Add --renderer gl3plus to compare OpenGL with the default D3D11.
REM cd /d C:\Run3-Game-Engine

"%~dp0build\install\windows-debug\bin\run3_shell.exe" ^
    --renderer d3d11 ^
    --content-root "C:\Run3-Game-Engine\Games\The Long Way\TheLongWay" ^
    --user-dir ".\build\user\windows-debug" ^
    --fullscreen ^
    --resolution 1920x1080 --fov 75 ^
    --texture-quality high --model-quality high --scene-quality high ^
    --audio-backend miniaudio ^
    %*

pause
