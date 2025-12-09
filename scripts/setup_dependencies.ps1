# scripts/setup_dependencies.ps1
$VendorPath = Join-Path $PSScriptRoot "..\vendor"
if (-not (Test-Path $VendorPath)) { New-Item -ItemType Directory -Path $VendorPath | Out-Null }

Write-Host "Setting up dependencies in $VendorPath..."

# 1. nlohmann/json
$JsonUrl = "https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp"
$JsonPath = Join-Path $VendorPath "json.hpp"
if (-not (Test-Path $JsonPath)) {
    Write-Host "Downloading json.hpp..."
    Invoke-WebRequest -Uri $JsonUrl -OutFile $JsonPath
}

# 2. cpp-httplib
$HttplibUrl = "https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h"
$HttplibPath = Join-Path $VendorPath "httplib.h"
if (-not (Test-Path $HttplibPath)) {
    Write-Host "Downloading httplib.h..."
    Invoke-WebRequest -Uri $HttplibUrl -OutFile $HttplibPath
}

# 3. Crow (crow_all.h)
$CrowUrl = "https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h"
$CrowPath = Join-Path $VendorPath "crow_all.h"
if (-not (Test-Path $CrowPath)) {
    Write-Host "Downloading crow_all.h..."
    Invoke-WebRequest -Uri $CrowUrl -OutFile $CrowPath
}

# 4. SQLite3
$SqliteUrl = "https://www.sqlite.org/2023/sqlite-amalgamation-3420000.zip"
$SqliteZip = Join-Path $VendorPath "sqlite.zip"
$SqliteExtracted = Join-Path $VendorPath "sqlite_extracted"

if (-not (Test-Path (Join-Path $VendorPath "sqlite3.c"))) {
    Write-Host "Downloading SQLite3..."
    Invoke-WebRequest -Uri $SqliteUrl -OutFile $SqliteZip
    Expand-Archive -Path $SqliteZip -DestinationPath $SqliteExtracted -Force
    
    # Move files up
    $AmalgamationFolder = Get-ChildItem $SqliteExtracted | Select-Object -First 1
    Move-Item (Join-Path $AmalgamationFolder.FullName "sqlite3.c") $VendorPath
    Move-Item (Join-Path $AmalgamationFolder.FullName "sqlite3.h") $VendorPath
    
    # Cleanup
    Remove-Item $SqliteZip
    Remove-Item $SqliteExtracted -Recurse
}

Write-Host "Dependencies setup complete."
Write-Host "NOTE: You still need OpenSSL installed on your system."
Write-Host "If you don't have it, try: winget install OpenSSL"
