@echo off
setlocal
python "%~dp0luniverse_map_viewer.py" --scene "%~dp0..\derived-content\luniverse\run3\maps\high\luniverse\scene.cfg" %*
if errorlevel 1 pause
