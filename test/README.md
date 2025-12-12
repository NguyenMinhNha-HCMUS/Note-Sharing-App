# Test Suite - Secure Note App

## Tổng quan

Thư mục này chứa các test cases để kiểm thử hệ thống Secure Note App. Các test bao gồm:

1. **Xác thực (Authentication)** - `test_authentication.md`
2. **Mã hóa/Giải mã (Encryption/Decryption)** - `test_encryption.md`
3. **Giới hạn Truy cập (Access Control)** - `test_access_control.md`
4. **Mã hóa Đầu-Cuối (End-to-End Encryption)** - `test_e2e_encryption.md`

## Cách sử dụng

### Phương pháp 1: Automated Tests với C++ (Khuyến nghị)

**Build và chạy:**

```powershell
# Cách 1: Dùng PowerShell script (tự động build + run)
.\test\build_and_run.ps1

# Cách 2: Build thủ công
g++ test/auto_test.cpp -o auto_test.exe -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00 -lws2_32 -lwsock32

# Chạy tests
.\auto_test.exe
```

**Điều chỉnh config trong `test/auto_test.cpp` (dòng 14-26):**
```cpp
const std::string SERVER_HOST = "localhost";
const int SERVER_PORT = 8080;
const int TOKEN_TTL_SECONDS = 60;  // Phải khớp với server

// Test users
std::map<std::string, TestUser> TEST_USERS = {
    {"alice", {"alice_test", "password123"}},
    {"bob", {"bob_test", "password456"}},
    {"charlie", {"charlie_test", "password789"}}
};

// Test configuration
const bool VERBOSE = true;
const bool TEST_EXPIRATION = true;  // Test token/link expiration
const int EXPIRATION_WAIT_TIME = 65;
```

**Output mẫu:**
```
============================================================
  SECURE NOTE APP - AUTOMATED TEST SUITE (C++)
============================================================
Server: http://localhost:8080
Token TTL: 60s

[TEST] 1.1 - Đăng ký thành công
✓ PASSED

[TEST] 1.2 - Đăng ký với username đã tồn tại
✓ Từ chối thành công

...

============================================================
FINAL RESULTS
============================================================
Total: 15/17 tests passed
Success Rate: 88.2%
🎉 ALL TESTS PASSED! 🎉
```

### Phương pháp 2: Automated Tests với Python

**Chạy tự động với Python:**

```powershell
# Cài đặt requests library (nếu chưa có)
pip install requests

# Chạy test
python test\auto_test.py

# Hoặc dùng PowerShell runner
.\test\run_tests.ps1
```

**Điều chỉnh config:**
- Sửa file `test/config.json` hoặc
- Sửa biến trong `test/auto_test.py` (dòng 11-21)

### Phương pháp 3: Manual Tests

1. **Chuẩn bị môi trường test**

```powershell
# Dừng server nếu đang chạy (Ctrl+C)
Remove-Item secure_notes.db

# Rebuild server với token TTL ngắn (để test expiration)
# Sửa Auth.cpp: TOKEN_TTL_SECONDS = 60
g++ -c server/Auth.cpp -o Auth.o -std=c++17 -I vendor
g++ server_main.o Auth.o Database.o Crypto.o sqlite3.o -o server_app.exe -lws2_32 -lwsock32 -lcrypto -lssl

# Khởi động server
.\server_app.exe
```

2. **Chạy test thủ công**

- Mở file test tương ứng (`.md`)
- Làm theo từng bước trong phần "Các bước"
- So sánh kết quả với "Kết quả mong đợi"
- Đánh dấu checkbox ở cuối mỗi test case

### Test users

Để test đầy đủ, cần tạo nhiều users:

```
User 1: alice / password123
User 2: bob / password456
User 3: charlie / password789
User 4: david / password000
```

## Checklist tổng hợp

### Category 1: Authentication (8 tests)
- [ ] 1.1 Đăng ký thành công
- [ ] 1.2 Duplicate username
- [ ] 1.3 Đăng nhập thành công
- [ ] 1.4 Password sai
- [ ] 1.5 Username không tồn tại
- [ ] 1.6 Token expired
- [ ] 1.7 Không có token
- [ ] 1.8 Password rỗng

### Category 2: Encryption (10 tests)
- [ ] 2.1 Text file encrypt/decrypt
- [ ] 2.2 Binary file integrity
- [ ] 2.3 Large file support
- [ ] 2.4 Key protection (wrapped)
- [ ] 2.5 Random IV
- [ ] 2.6 Wrong key rejection
- [ ] 2.7 Receive key encrypted
- [ ] 2.8 Load receive key
- [ ] 2.9 AES-256-CBC verified
- [ ] 2.10 PBKDF2 verified

### Category 3: Access Control (10 tests)
- [ ] 3.1 Expired link rejected
- [ ] 3.2 Whitelist enforcement
- [ ] 3.3 Owner can delete
- [ ] 3.4 Non-owner cannot delete
- [ ] 3.5 Cannot download other's notes
- [ ] 3.6 Owner can revoke
- [ ] 3.7 Non-owner cannot revoke
- [ ] 3.8 Invalid token rejected
- [ ] 3.9 Multiple users whitelist
- [ ] 3.10 Mid-use expiration

### Category 4: E2E Encryption (12 tests)
- [ ] 4.1 ECDH key exchange
- [ ] 4.2 Ephemeral key cleanup
- [ ] 4.3 Server cannot decrypt
- [ ] 4.4 Forward secrecy
- [ ] 4.5 Receiver key usage
- [ ] 4.6 Wrong key rejection
- [ ] 4.7 secp256k1 curve
- [ ] 4.8 Multiple recipients
- [ ] 4.9 Session key cleanup
- [ ] 4.10 Key wrapping
- [ ] 4.11 Full E2E workflow
- [ ] 4.12 Public key format

**Tổng cộng**: 40 test cases

## Tools hỗ trợ

### SQLite query
```powershell
# Xem users
sqlite3 secure_notes.db "SELECT id, username FROM Users;"

# Xem notes
sqlite3 secure_notes.db "SELECT id, user_id, filename, created_at FROM Notes;"

# Xem share links
sqlite3 secure_notes.db "SELECT token, note_id, expiration_time FROM SharedLinks;"

# Xem share access
sqlite3 secure_notes.db "SELECT username, send_public_key_hex FROM SharedLinkAccess;"
```

### File hash
```powershell
Get-FileHash upload/test.pdf
Get-FileHash download/test.pdf
```

### Check file size
```powershell
(Get-Item upload/test.pdf).Length
(Get-Item download/test.pdf).Length
```

## Ghi chú

- Test theo thứ tự từ 1 → 4 (dependencies)
- Một số test yêu cầu sửa code tạm thời (add debug logs)
- Test 1.6 và 3.1, 3.10 yêu cầu đợi timeout
- Test 4.6 có thể cần bypass whitelist check tạm thời để test decrypt failure

## Báo cáo kết quả

Sau khi hoàn thành, tổng hợp:
- Số test passed / total
- Các test failed (nếu có) và lý do
- Bugs phát hiện (nếu có)
- Đề xuất cải thiện

---

**Ngày bắt đầu**: _______________  
**Người test**: _______________  
**Kết quả**: _____ / 40 tests passed
