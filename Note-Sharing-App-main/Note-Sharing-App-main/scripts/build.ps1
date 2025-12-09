# scripts/build.ps1
$ProjectRoot = Join-Path $PSScriptRoot ".."
$VendorPath = Join-Path $ProjectRoot "vendor"
$CommonPath = Join-Path $ProjectRoot "common"
$ServerPath = Join-Path $ProjectRoot "server"
$ClientPath = Join-Path $ProjectRoot "client"

# Check for g++
if (-not (Get-Command "g++" -ErrorAction SilentlyContinue)) {
    Write-Error "g++ not found. Please install MinGW or ensure g++ is in your PATH."
    exit 1
}

# Auto-detect OpenSSL installation
$OpenSSLInclude = ""
$OpenSSLLib = ""

# Check common OpenSSL locations
$PossiblePaths = @(
    "C:\Program Files\OpenSSL-Win64",
    "C:\Program Files\OpenSSL",
    "C:\OpenSSL-Win64",
    "C:\msys64\mingw64",
    "C:\Program Files\Git\mingw64"
)

foreach ($path in $PossiblePaths) {
    if (Test-Path (Join-Path $path "include\openssl")) {
        $OpenSSLInclude = "-I`"$path\include`""
        $OpenSSLLib = "-L`"$path\lib`""
        Write-Host "Found OpenSSL at: $path"
        break
    }
}

if (-not $OpenSSLInclude) {
    Write-Warning "OpenSSL not found in common locations. Trying system PATH..."
    Write-Warning "If build fails, install OpenSSL: winget install ShiningLight.OpenSSL.Dev"
}

Write-Host "Building Server..."
$ServerSrc = @(
    (Join-Path $ServerPath "server_main.cpp"),
    (Join-Path $ServerPath "Database.cpp"),
    (Join-Path $ServerPath "Auth.cpp"),
    (Join-Path $CommonPath "Crypto.cpp"),
    (Join-Path $VendorPath "sqlite3.c")
)
$ServerOut = Join-Path $ProjectRoot "server_app.exe"

# Link flags for Windows (ws2_32 for winsock, crypt32/ssl for openssl)
$Libs = "-lws2_32 -lssl -lcrypto -lpthread" 

$BuildCmd = "g++ -std=c++17 $OpenSSLInclude -I$VendorPath -I$CommonPath -I$ServerPath $ServerSrc -o $ServerOut $OpenSSLLib $Libs"
Write-Host $BuildCmd
Invoke-Expression $BuildCmd

if ($LASTEXITCODE -eq 0) { Write-Host "Server built successfully: $ServerOut" }
else { Write-Error "Server build failed"; exit 1 }

Write-Host "Building Client..."
$ClientSrc = @(
    (Join-Path $ProjectRoot "client_main.cpp"),
    (Join-Path $ClientPath "client_app_logic.cpp"),
    (Join-Path $ClientPath "network.cpp"),
    (Join-Path $CommonPath "Crypto.cpp")
)
$ClientOut = Join-Path $ProjectRoot "client_app.exe"

$BuildCmd = "g++ -std=c++17 $OpenSSLInclude -I$VendorPath -I$CommonPath -I$ClientPath $ClientSrc -o $ClientOut $OpenSSLLib $Libs"
Write-Host $BuildCmd
Invoke-Expression $BuildCmd

if ($LASTEXITCODE -eq 0) { Write-Host "Client built successfully: $ClientOut" }
else { Write-Error "Client build failed"; exit 1 }
