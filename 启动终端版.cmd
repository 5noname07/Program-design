@echo off
cd /d "%~dp0"
if exist "dist-console\canteen.exe" (
    "dist-console\canteen.exe" "%~dp0dist-console\data\canteen.txt"
) else if exist "build-console\canteen.exe" (
    "build-console\canteen.exe" "%~dp0data\canteen.txt"
) else if exist "canteen.exe" (
    "canteen.exe" "%~dp0data\canteen.txt"
) else (
    echo Please download the Console release or build the console target first.
)
pause
