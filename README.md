# Secure Note Sharing Application

Ứng dụng chia sẻ ghi chú an toàn với mã hóa end-to-end (E2EE) sử dụng ECDH và AES-256-CBC.

## ✨ Tính năng chính

- 🔐 **End-to-End Encryption**: Mã hóa hoàn toàn từ người gửi đến người nhận
- 🔑 **ECDH Key Exchange**: Trao đổi khóa an toàn với secp256k1
- 📝 **Quản lý Note**: Tạo, xem, xóa ghi chú được mã hóa
- 🔗 **Chia sẻ linh hoạt**: Share link hoặc trực tiếp cho user
- 📁 **Hỗ trợ file**: Upload/download bất kỳ loại file nào với tên file gốc
- ⏰ **Thời gian hết hạn**: Tự động hết hạn link/share sau thời gian định trước
- 🧪 **Test Suite**: Bộ test tự động bằng C++ với 17+ test cases
- 🔒 **JWT Authentication**: Xác thực token với TTL 30 phút

## 📋 Yêu cầu hệ thống

### Cài đặt môi trường (Windows)

#### 1. **C++ Compiler - GCC 14.2.0 trở lên**

**Khuyến nghị: MSYS2** (bao gồm g++, OpenSSL, và các thư viện cần thiết)

```powershell
# Cài MSYS2
winget install MSYS2.MSYS2
```

Sau khi cài xong, mở **MSYS2 UCRT64** terminal và chạy:
```bash
# Cập nhật package database
pacman -Syu

# Cài compiler và OpenSSL
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-openssl
```

**Thêm vào PATH** (quan trọng!):
```powershell
# PowerShell - thêm tạm thời
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"

# Hoặc thêm vĩnh viễn qua System Properties > Environment Variables
# Thêm: C:\msys64\ucrt64\bin  (điều chỉnh đường dẫn nếu cài ở chỗ khác)
```

#### 2. **Kiểm tra cài đặt**

```powershell
# Kiểm tra g++ (cần >= 14.2.0 để hỗ trợ C++17 đầy đủ)
g++ --version

# Kiểm tra OpenSSL
openssl version
```

Kết quả mong đợi:
```
g++ (Rev2, Built by MSYS2 project) 14.2.0
OpenSSL 3.x.x
```

---

## 🚀 Cách chạy chương trình

### Bước 1: Build (Biên dịch)

**Cách nhanh nhất - Build tất cả bằng 1 lệnh:**
```powershell
.\build_all.ps1
```

**Hoặc build lại từ đầu:**
```powershell
.\build_all.ps1 -Clean
```

Script này sẽ tự động:
- ✅ Biên dịch server (server_app.exe)
- ✅ Biên dịch client (client_app.exe)
- ✅ Hiển thị tiến trình từng bước
- ✅ Báo lỗi chi tiết nếu có

### Bước 2: Chạy ứng dụng

**Mở 2 terminal riêng biệt:**

**Terminal 1 - Chạy Server:**
```powershell
.\server_app.exe
```
Server sẽ lắng nghe trên `http://localhost:8080`

**Terminal 2 - Chạy Client:**
```powershell
.\client_app.exe
```

### Bước 3: Sử dụng

1. **Đăng ký tài khoản** (chọn option 1)
2. **Đăng nhập** (chọn option 2)
3. Sử dụng các chức năng:
   - Tạo note mới
   - Xem danh sách notes
   - Share note (qua link hoặc trực tiếp)
   - Xem notes được share cho mình
   - Download note dưới dạng file
   - Xem danh sách notes đã share cho người khác

---

## 🧪 Chạy Test Suite

### Build test:
```powershell
g++ test/auto_test.cpp -o auto_test.exe -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00 -lws2_32 -lwsock32 -lcrypt32
```

### Cấu hình test:

Chỉnh sửa file `test/test_config.json` để thay đổi các thông số test:

```json
{
  "server": {
    "host": "localhost",
    "port": 8080
  },
  "token": {
    "ttl_seconds": 60,
    "expiration_wait_time": 65
  },
  "test_users": {
    "alice": {
      "username": "alice_test",
      "password": "password123"
    },
    "bob": {...},
    "charlie": {...}
  },
  "test_options": {
    "verbose": true,
    "test_expiration": true
  }
}
```

### Chạy test (cần server đang chạy):
```powershell
# Chạy test với config từ test/test_config.json
.\auto_test.exe

# Xem help
.\auto_test.exe --help
```

### Hoặc dùng script tự động:
```powershell
.\test\build_and_run.ps1
```
Script này sẽ tự động khởi động server, reset database, và chạy test.

**Lưu ý:** Test config độc lập với server. Thay đổi config không cần rebuild test.

**Xem hướng dẫn test chi tiết:** [test/README.md](test/README.md)

---

## 📖 Tài liệu chi tiết

- **[HUONG_DAN_SU_DUNG.md](HUONG_DAN_SU_DUNG.md)** - Hướng dẫn sử dụng đầy đủ
- **[Project explain.md](Project%20explain.md)** - Giải thích kiến trúc và cơ chế mã hóa
- **[test/README.md](test/README.md)** - Hướng dẫn test và 40 test cases thủ công
- **[test/test_config.json](test/test_config.json)** - File cấu hình test (có thể chỉnh sửa)

---

## ⚙️ Cấu hình nâng cao

### Token Time-to-Live (TTL)

**Server:**
- Token TTL được cố định: **30 phút (1800 giây)**
- Để thay đổi, chỉnh trực tiếp trong [server/Auth.cpp](server/Auth.cpp#L11):
  ```cpp
  static const long long TOKEN_TTL_SECONDS = 1800; // 30 minutes
  ```
- Sau khi thay đổi, rebuild server: `.\build_all.ps1`

**Test:**
- Test có thể dùng TTL khác để kiểm tra expiration
- Chỉnh trong `test/test_config.json`:
  ```json
  "token": {
    "ttl_seconds": 60,
    "expiration_wait_time": 65
  }
  ```
- Không cần rebuild test sau khi thay đổi config

### Reset Database
```powershell
# Xóa database để bắt đầu lại
Remove-Item secure_notes.db -Force
```

### Clean Build Artifacts
```powershell
# Xóa tất cả file .o và .exe
Remove-Item *.o, *.exe -Force

# Hoặc dùng build script
.\build_all.ps1 -Clean
```

---

## 🏗️ Cấu trúc Project

```
Project/
├── server/              # Server code
│   ├── server_main.cpp  # API endpoints (14 APIs)
│   ├── Auth.cpp         # JWT authentication (TTL: 30 min)
│   └── Database.cpp     # SQLite operations
├── client/              # Client code
│   ├── client_app_logic.cpp
│   └── network.cpp      # HTTP client
├── common/
│   ├── Crypto.cpp       # ECDH, AES-256-CBC, PBKDF2
│   └── Protocol.h       # Shared data structures
├── test/                # Test suite
│   ├── auto_test.cpp    # 17+ automated tests
│   ├── test_config.json # Test configuration file
│   ├── build_and_run.ps1
│   └── *.md             # 40 manual test cases
├── vendor/              # Dependencies
│   ├── crow_all.h       # HTTP server framework
│   ├── httplib.h        # HTTP client library
│   ├── json.hpp         # JSON parser
│   └── sqlite3.*        # Database
├── build_all.ps1        # Build script chính (1 lệnh build all)
├── secure_notes.db      # SQLite database (tự tạo khi chạy server)
└── README.md            # File này
```

---

## 🔧 Công nghệ sử dụng

| Thành phần | Công nghệ |
|------------|-----------|
| **Language** | C++17 |
| **Compiler** | GCC 14.2.0 (MSYS2) |
| **Server Framework** | Crow (header-only) |
| **HTTP Client** | cpp-httplib |
| **Database** | SQLite3 |
| **JSON** | nlohmann/json |
| **Encryption** | OpenSSL 3.x |
| **Key Exchange** | ECDH (secp256k1) |
| **Symmetric Encryption** | AES-256-CBC |
| **Key Derivation** | PBKDF2-SHA256 (10k iterations) |
| **Authentication** | JWT with configurable TTL |

---

## ❓ Xử lý lỗi thường gặp

### Lỗi: "g++ not found" hoặc "command not found"

**Nguyên nhân:** Chưa thêm g++ vào PATH

**Giải pháp:**
```powershell
# Kiểm tra PATH hiện tại
$env:PATH

# Thêm tạm thời (session hiện tại)
# Thay đổi đường dẫn theo nơi bạn cài MSYS2
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"

# Hoặc thêm vĩnh viễn:
# 1. Windows Search > "Environment Variables"
# 2. System Properties > Environment Variables
# 3. Thêm: C:\msys64\ucrt64\bin vào PATH (hoặc đường dẫn MSYS2 của bạn)
```

### Lỗi: "OpenSSL headers not found"

**Nguyên nhân:** Chưa cài OpenSSL hoặc chưa có trong PATH

**Giải pháp:**
```bash
# Cài OpenSSL qua MSYS2
pacman -S mingw-w64-ucrt-x86_64-openssl

# Kiểm tra
openssl version
```

### Lỗi: "cannot find -lcrypto" hoặc "-lssl"

**Nguyên nhân:** Thiếu OpenSSL libraries

**Giải pháp:** Cài lại OpenSSL hoặc kiểm tra PATH đúng folder (ucrt64/bin)

### Lỗi Build: "undefined reference to..."

**Nguyên nhân:** Thiếu library khi linking

**Giải pháp:** Đảm bảo lệnh build có đủ các flags:
- `-lws2_32 -lwsock32` (Windows sockets)
- `-lcrypto -lssl` (OpenSSL)
- `-lcrypt32` (Windows crypto - cho client)

### Server không start được

```powershell
# Kiểm tra port 8080 có bị chiếm không
netstat -ano | findstr :8080

# Nếu có process đang dùng, kill nó:
Stop-Process -Id <PID> -Force
```

### Client không kết nối được server

1. Đảm bảo server đã chạy và hiển thị "Server running on port 8080"
2. Kiểm tra firewall không block port 8080
3. Thử truy cập: http://localhost:8080 trên browser

### IntelliSense báo lỗi (red squiggles) nhưng vẫn compile được

**Đây chỉ là warning của VS Code IntelliSense**, code vẫn chạy bình thường.

**Để fix hoàn toàn:**
1. Mở `.vscode/c_cpp_properties.json`
2. Chỉnh `includePath` đúng với thư mục OpenSSL của bạn
3. Xóa các path không tồn tại

---

## 📚 API Endpoints (14 APIs)

| Method | Endpoint | Mô tả | Auth |
|--------|----------|-------|------|
| POST | `/register` | Đăng ký user mới | ❌ |
| POST | `/login` | Đăng nhập | ❌ |
| POST | `/create_note` | Tạo note mới | ✅ |
| GET | `/my_notes` | Xem notes của mình | ✅ |
| DELETE | `/delete_note/:id` | Xóa note | ✅ |
| GET | `/note/:id` | Xem note theo ID | ✅ |
| POST | `/share_note` | Share note cho user | ✅ |
| GET | `/shared_with_me` | Notes được share cho mình | ✅ |
| POST | `/create_shared_link` | Tạo share link | ✅ |
| GET | `/link/:token` | Truy cập note qua link | ❌ |
| POST | `/download_note` | Download note dưới dạng file | ✅ |
| GET | `/download_link/:token` | Download qua link | ❌ |
| GET | `/shared_links` | Danh sách link đã tạo | ✅ |
| GET | `/myshares` | Notes đã share cho người khác | ✅ |

---

## 👨‍💻 Development

### Build từng thành phần riêng lẻ

**Server:**
```powershell
gcc -c vendor/sqlite3.c -o sqlite3.o
g++ -c server/server_main.cpp -o server_main.o -std=c++17 -I vendor/asio_lib -I vendor
g++ -c server/Auth.cpp -o Auth.o -std=c++17 -I vendor
g++ -c server/Database.cpp -o Database.o -std=c++17 -I vendor
g++ -c common/Crypto.cpp -o Crypto.o -std=c++17 -I vendor
g++ server_main.o Auth.o Database.o Crypto.o sqlite3.o -o server_app.exe -lws2_32 -lwsock32 -lcrypto -lssl
```

**Client:**
```powershell
g++ -c client_main.cpp -o client_main.o -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00
g++ -c client/client_app_logic.cpp -o client_app_logic.o -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00
g++ -c client/network.cpp -o network.o -std=c++17 -I vendor -D_WIN32_WINNT=0x0A00
g++ client_main.o client_app_logic.o network.o Crypto.o -o client_app.exe -lws2_32 -lwsock32 -lcrypto -lssl -lcrypt32
```

### Debug mode (xem warnings chi tiết)

```powershell
# Xóa '2>$null' trong build_all.ps1 để xem output đầy đủ
# Hoặc build thủ công với output
g++ -c server/Auth.cpp -o Auth.o -std=c++17 -I vendor
```

---

## 📝 License

Educational project for Cryptography Lab 02.

---

## 🤝 Credits

- **Crow Framework** - HTTP server
- **cpp-httplib** - HTTP client
- **nlohmann/json** - JSON parsing
- **OpenSSL** - Cryptographic operations
- **SQLite** - Database engine