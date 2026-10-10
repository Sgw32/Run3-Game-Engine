@echo off
REM Step 9B preview: extra arguments override the defaults below.
REM run_tlw.bat --lighting-pipeline legacy-forward --shadow-quality low
REM run_tlw.bat --lighting-pipeline fast-forward --shadow-quality off
REM run_tlw.bat --lighting-pipeline pbr --shadow-quality high --exposure 1
REM run_tlw.bat --lighting-pipeline deferred --shadow-quality low
REM These new pipelines are experimental; campaign validation is still in progress.
REM TLWRM overlays run3/core, run3/maps, run3/game and run3/shaders.
REM Existing files there replace originals; absent files fall back to original content.
REM To run only original content, remove the two TLWRM arguments below or call
REM run3_shell.exe directly with --content-variant original.
REM Add --map tlwhome02 or --renderer gl3plus to override the map or renderer.
REM cd /d C:\Run3-Game-Engine

"%~dp0build\install\windows-release\bin\run3_shell.exe" ^
    --renderer d3d11 ^
    --content-root "C:\Run3-Game-Engine\Games\The Long Way\TheLongWay" ^
    --content-variant tlwrm ^
    --content-overlay "%~dp0derived-content\tlwrm" ^
    --map tlwintro ^
    --user-dir ".\build\user\windows-release" ^
    --fullscreen ^
    --lighting-pipeline legacy-forward --shadow-quality low --exposure 0.6 ^
    --resolution 1920x1080 --fov 75 ^
    --texture-quality high --model-quality high --scene-quality high ^
    --audio-backend miniaudio ^
    %*

pause
