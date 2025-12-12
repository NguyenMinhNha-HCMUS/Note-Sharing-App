# Build script for both server and client
param(
    [switch]$Clean
)

Write-Host "=======================================" -ForegroundColor Cyan
Write-Host "  Secure Note - Build Script" -ForegroundColor Cyan
Write-Host "=======================================" -ForegroundColor Cyan
Write-Host ""

# Clean if requested
if ($Clean) {
    Write-Host "Cleaning old build artifacts..." -ForegroundColor Yellow
    Remove-Item *.o -ErrorAction SilentlyContinue
    Remove-Item *.exe -ErrorAction SilentlyContinue
    Write-Host "Cleaned" -ForegroundColor Green
    Write-Host ""
}

$ErrorActionPreference = "Continue"
$buildFailed = $false

# Build Server
Write-Host "=======================================" -ForegroundColor Cyan
Write-Host "  Building Server..." -ForegroundColor Cyan
Write-Host "=======================================" -ForegroundColor Cyan

Write-Host "[1/6] Compiling sqlite3.c..." -NoNewline
gcc -c vendor/sqlite3.c -o sqlite3.o 2>$null
if ($LASTEXITCODE -eq 0) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " FAIL" -ForegroundColor Red; $buildFailed = $true }

Write-Host "[2/6] Compiling server_main.cpp..." -NoNewline
g++ -c server/server_main.cpp -o server_main.o -std=c++17 -I vendor/asio_lib -I vendor 2>$null
if ($LASTEXITCODE -eq 0) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " FAIL" -ForegroundColor Red; $buildFailed = $true }

Write-Host "[3/6] Compiling Auth.cpp..." -NoNewline
g++ -c server/Auth.cpp -o Auth.o -std=c++17 -I vendor 2>$null
if ($LASTEXITCODE -eq 0) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " FAIL" -ForegroundColor Red; $buildFailed = $true }

Write-Host "[4/6] Compiling Database.cpp..." -NoNewline
g++ -c server/Database.cpp -o Database.o -std=c++17 -I vendor 2>$null
if ($LASTEXITCODE -eq 0) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " FAIL" -ForegroundColor Red; $buildFailed = $true }

Write-Host "[5/6] Compiling Crypto.cpp..." -NoNewline
g++ -c common/Crypto.cpp -o Crypto.o -std=c++17 -I vendor 2>$null
if ($LASTEXITCODE -eq 0) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " FAIL" -ForegroundColor Red; $buildFailed = $true }

Write-Host "[6/6] Linking server_app.exe..." -NoNewline
g++ server_main.o Auth.o Database.o Crypto.o sqlite3.o -o server_app.exe -lws2_32 -lwsock32 -lcrypto -lssl 2>$null
if ($LASTEXITCODE -eq 0) { 
    Write-Host " OK" -ForegroundColor Green 
    Write-Host ""
    Write-Host "Server build successful: server_app.exe" -ForegroundColor Green
    Write-Host ""
} else { 
    Write-Host " FAIL" -ForegroundColor Red
    Write-Host "Server build failed" -ForegroundColor Red
    Write-Host ""
    $buildFailed = $true 
}

# Build Client
Write-Host "=======================================" -ForegroundColor Cyan
Write-Host "  Building Client..." -ForegroundColor Cyan
Write-Host "=======================================" -ForegroundColor Cyan

Write-Host "[1/4] Compiling client_main.cpp..." -NoNewline
g++ -c client_main.cpp -o client_main.o -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00 2>$null
if ($LASTEXITCODE -eq 0) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " FAIL" -ForegroundColor Red; $buildFailed = $true }

Write-Host "[2/4] Compiling client_app_logic.cpp..." -NoNewline
g++ -c client/client_app_logic.cpp -o client_app_logic.o -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00 2>$null
if ($LASTEXITCODE -eq 0) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " FAIL" -ForegroundColor Red; $buildFailed = $true }

Write-Host "[3/4] Compiling network.cpp..." -NoNewline
g++ -c client/network.cpp -o network.o -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00 2>$null
if ($LASTEXITCODE -eq 0) { Write-Host " OK" -ForegroundColor Green } else { Write-Host " FAIL" -ForegroundColor Red; $buildFailed = $true }

Write-Host "[4/4] Linking client_app.exe..." -NoNewline
g++ client_main.o client_app_logic.o network.o Crypto.o -o client_app.exe -lws2_32 -lwsock32 -lcrypto -lssl -lcrypt32 2>$null
if ($LASTEXITCODE -eq 0) { 
    Write-Host " OK" -ForegroundColor Green 
    Write-Host ""
    Write-Host "Client build successful: client_app.exe" -ForegroundColor Green
    Write-Host ""
} else { 
    Write-Host " FAIL" -ForegroundColor Red
    Write-Host "Client build failed" -ForegroundColor Red
    Write-Host ""
    $buildFailed = $true 
}

# Summary
Write-Host "=======================================" -ForegroundColor Cyan
if ($buildFailed) {
    Write-Host "Build completed with errors" -ForegroundColor Red
    Write-Host "Run without '2>null' to see error details" -ForegroundColor Yellow
    exit 1
} else {
    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "To run:" -ForegroundColor Cyan
    Write-Host "  Terminal 1: .\server_app.exe" -ForegroundColor Yellow
    Write-Host "  Terminal 2: .\client_app.exe" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To clean and rebuild: .\build_all.ps1 -Clean" -ForegroundColor Yellow
    exit 0
}
