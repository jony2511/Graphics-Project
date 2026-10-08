@echo off
taskkill /F /IM main.exe >nul 2>&1
echo Compiling OpenGL Project...
g++ -std=c++17 src/*.cpp src/glad.c -Iinclude -Llib -lglfw3 -lopengl32 -lgdi32 -o main.exe
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Build succeeded! Running main.exe...
    main.exe
) else (
    echo [ERROR] Build failed!
)
