# OpenGL Blank Starter Project

A clean OpenGL (3.3+ Core Profile) starter template configured with **GLFW** and **GLAD** for Windows (MinGW / GCC / MSYS2 UCRT64).

---

## 📁 Project Structure

```text
gr/
├── .vscode/                 # VS Code IntelliSense & Build configurations
│   ├── c_cpp_properties.json
│   └── tasks.json
├── include/                 # Header files
│   ├── glad/gl.h            # GLAD OpenGL loader header
│   ├── GLFW/glfw3.h         # GLFW window/input library header
│   └── KHR/khrplatform.h
├── lib/                     # Static & dynamic library binaries (64-bit)
│   ├── libglfw3.a
│   └── glfw3.dll
├── src/
│   ├── glad.c               # GLAD implementation
│   └── main.cpp             # Starter OpenGL window loop
├── CMakeLists.txt           # Optional CMake configuration
├── Makefile                 # Make build script
├── run.bat                  # 1-click build and run script
└── README.md
```

---

## 🚀 How to Build and Run

### Option 1: Using the 1-Click Batch Script
Double-click or run in terminal:
```cmd
run.bat
```

### Option 2: Using g++ in Terminal
```bash
g++ -std=c++17 src/main.cpp src/glad.c -Iinclude -Llib -lglfw3 -lopengl32 -lgdi32 -o main.exe
./main.exe
```

### Option 3: Using Makefile
```bash
mingw32-make run
```

### Option 4: Inside VS Code
1. Open the project folder in VS Code.
2. Press `Ctrl + Shift + B` to trigger the build task.
3. Run `./main.exe` in the integrated terminal.
