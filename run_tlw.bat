@echo off
REM Step 9B preview: extra arguments override the defaults below.
REM run_tlw.bat --lighting-pipeline legacy-forward --shadow-quality low
REM run_tlw.bat --lighting-pipeline fast-forward --shadow-quality off
REM run_tlw.bat --lighting-pipeline pbr --shadow-quality high --exposure 1
REM run_tlw.bat --lighting-pipeline deferred --shadow-quality low
REM These new pipelines are experimental; campaign validation is still in progress.
REM Content overlay is independent of the lighting pipeline. Use an existing hashed copy:
REM run_tlw.bat --lighting-pipeline pbr --content-variant nextgen --content-overlay "%~dp0derived-content\nextgen-v0"
REM nextgen-v0 is currently a byte-identical staging copy, NOT a lighting retune.
REM Use --content-variant original without --content-overlay for original content.
REM Add --map tlwhome02 or --renderer gl3plus to override the map or renderer.
REM cd /d C:\Run3-Game-Engine

"%~dp0build\install\windows-release\bin\run3_shell.exe" ^
    --renderer d3d11 ^
    --content-root "C:\Run3-Game-Engine\Games\The Long Way\TheLongWay" ^
    --map tlwcao ^
    --user-dir ".\build\user\windows-release" ^
    --fullscreen ^
    --lighting-pipeline deferred --shadow-quality high --exposure 0.6 ^
    --resolution 1920x1080 --fov 75 ^
    --texture-quality low --model-quality low --scene-quality high ^
    --audio-backend miniaudio ^
    %*

pause
