# Secure Note App

A secure, end-to-end encrypted note sharing application with end-to-end encryption.

## Features

- User registration and authentication
- End-to-end encryption using AES-256
- Secure file sharing
- Password hashing with SHA-256
- REST API server with Crow framework

## Installation Guide

### Prerequisites

You need these installed **BEFORE** running the setup:

#### 1. C++ Compiler (MinGW/g++)

Choose one of these options:

**Option A: MSYS2 (Recommended)**
```powershell
winget install MSYS2.MSYS2
```
After installation, open MSYS2 terminal and run:
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl
```
Then add to PATH: `C:\msys64\mingw64\bin`

**Option B: Git for Windows (includes MinGW)**
```powershell
winget install Git.Git
```

#### 2. OpenSSL

Required for encryption. Choose one:

**Option A: Standalone OpenSSL**
```powershell
winget install ShiningLight.OpenSSL.Dev
```

**Option B: Already included if you installed:**
- Git for Windows
- MSYS2 (with the openssl package above)

#### 3. Verify Installation
```powershell
g++ --version      # Should show GCC version
```

### Build and Run

```powershell
.\install_and_build.bat
```
This will:
1. Download dependencies (SQLite, JSON, httplib, Crow)
2. Auto-detect OpenSSL location
3. Build server and client

#### Run Applications
```powershell
# Terminal 1 - Run Server
.\server_app.exe

# Terminal 2 - Run Client
.\client_app.exe
```

---

## Troubleshooting

### "OpenSSL headers not found"

The build script auto-detects OpenSSL in common locations:
- `C:\Program Files\OpenSSL-Win64`
- `C:\Program Files\Git\mingw64`
- `C:\msys64\mingw64`

If you installed OpenSSL elsewhere, either:
1. Add it to your system PATH
2. Create a symbolic link to a standard location
3. Manually edit `scripts/build.ps1` to add your path

### VS Code IntelliSense Errors (Red Squiggles)

The red squiggles for OpenSSL includes are just IntelliSense warnings. **The code will still compile correctly.**

The `.vscode/c_cpp_properties.json` file lists multiple possible OpenSSL locations. IntelliSense will use the first one it finds.

**To fix completely:**
1. Open `.vscode/c_cpp_properties.json`
2. Keep only the path where YOUR OpenSSL is installed
3. Remove the other paths that don't exist on your system

### "g++ not found"

Make sure MinGW's bin directory is in your PATH:
```powershell
# Check current PATH
$env:PATH

# Add MinGW to PATH (example for MSYS2)
$env:PATH += ";C:\msys64\mingw64\bin"

# To make permanent, add via System Properties > Environment Variables
```

## Development Notes

- The project uses standard C++17
- OpenSSL 3.x is supported
- Dependencies are automatically managed via scripts
- Cross-platform compatible (Windows focus, but adaptable)