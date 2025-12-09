# Windows Server Setup Guide

Quick guide to build and run the server on Windows.

## Prerequisites

Install these **before** building:

1. **C++ Compiler (MinGW)**
   ```powershell
   winget install MSYS2.MSYS2
   ```
   Then open MSYS2 terminal and run:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl
   ```
   Add to PATH: `C:\msys64\mingw64\bin`

2. **OpenSSL** (if not installed with MSYS2)
   ```powershell
   winget install ShiningLight.OpenSSL.Dev
   ```

3. **Verify installation**
   ```powershell
   g++ --version
   ```

## Build and Run

1. **Build the server:**
   ```powershell
   .\install_and_build.bat
   ```

2. **Run the server:**
   ```powershell
   .\server_app.exe
   ```

The server will start on `http://localhost:8080`

## Quick Test

Visit `http://localhost:8080/` in your browser to see available API endpoints.

## Troubleshooting

- **"g++ not found"**: Add MinGW to PATH: `C:\msys64\mingw64\bin`
- **"OpenSSL not found"**: Install OpenSSL or ensure it's in one of these locations:
  - `C:\Program Files\OpenSSL-Win64`
  - `C:\msys64\mingw64`
  - `C:\Program Files\Git\mingw64`

## API Endpoints

- `POST /register` - Register new user
- `POST /login` - User login  
- `POST /upload` - Upload encrypted note (auth required)
- `GET /notes` - List user's notes (auth required)
- `GET /note/<id>` - Get note by ID (auth required)
- And more... (see `http://localhost:8080/` for full list)

