# Build and Run Automated Tests (C++)
# Chạy: .\test\build_and_run.ps1

Write-Host "=" * 60 -ForegroundColor Blue
Write-Host "  BUILD AND RUN C++ TEST SUITE" -ForegroundColor Blue
Write-Host "=" * 60 -ForegroundColor Blue
Write-Host ""

# Check if server is running
Write-Host "[CHECK] Kiểm tra server..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri "http://localhost:8080" -TimeoutSec 3 -ErrorAction Stop
    Write-Host "✓ Server đang chạy" -ForegroundColor Green
} catch {
    Write-Host "✗ Server không phản hồi!" -ForegroundColor Red
    Write-Host "  Khởi động server: .\server_app.exe" -ForegroundColor Yellow
    
    $startServer = Read-Host "Có muốn khởi động server tự động? (y/n)"
    if ($startServer -eq "y") {
        Write-Host "[START] Khởi động server..." -ForegroundColor Yellow
        Start-Process -FilePath ".\server_app.exe" -WindowStyle Normal
        Write-Host "Đợi 3 giây để server khởi động..." -ForegroundColor Yellow
        Start-Sleep -Seconds 3
    } else {
        exit 1
    }
}

# Check if database should be reset
if (Test-Path "secure_notes.db") {
    Write-Host "[CHECK] Database tồn tại" -ForegroundColor Yellow
    $resetDb = Read-Host "Có muốn reset database? (y/n)"
    if ($resetDb -eq "y") {
        Write-Host "[RESET] Xóa database cũ..." -ForegroundColor Yellow
        Get-Process -Name "server_app" -ErrorAction SilentlyContinue | Stop-Process -Force
        Remove-Item "secure_notes.db" -Force
        Write-Host "✓ Database đã xóa" -ForegroundColor Green
        
        Write-Host "[RESTART] Khởi động lại server..." -ForegroundColor Yellow
        Start-Process -FilePath ".\server_app.exe" -WindowStyle Normal
        Start-Sleep -Seconds 3
    }
}

Write-Host ""
Write-Host "[BUILD] Biên dịch test suite..." -ForegroundColor Cyan

# Compile auto_test.cpp
$compileCmd = "g++ test/auto_test.cpp -o auto_test.exe -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00 -lws2_32 -lwsock32"
Write-Host "Command: $compileCmd" -ForegroundColor Gray

Invoke-Expression $compileCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "✗ Biên dịch thất bại!" -ForegroundColor Red
    Write-Host "Kiểm tra lỗi compiler ở trên." -ForegroundColor Yellow
    exit 1
}

Write-Host "✓ Biên dịch thành công" -ForegroundColor Green
Write-Host ""

# Run tests
Write-Host "[RUN] Chạy automated tests..." -ForegroundColor Cyan
Write-Host ""

.\auto_test.exe

$exitCode = $LASTEXITCODE

Write-Host ""
Write-Host "=" * 60 -ForegroundColor Blue

if ($exitCode -eq 0) {
    Write-Host "✓ All tests passed!" -ForegroundColor Green
} else {
    Write-Host "✗ Some tests failed (exit code: $exitCode)" -ForegroundColor Red
}

Write-Host "=" * 60 -ForegroundColor Blue

exit $exitCode
