@echo off

SET VC_DIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build

call "%VC_DIR%\vcvarsall.bat" amd64

if exist build (echo "build folder exist.") else (md build)
cd build/

cmake ../
cd ../

pause
goto :EOF
