# Ứng dụng Ghi chú Bảo mật - Tài liệu Giao thức & Logic

Tài liệu này mô tả kiến trúc, các tiêu chuẩn bảo mật và luồng xử lý logic được sử dụng trong Ứng dụng Ghi chú Bảo mật (Secure Note App). Tài liệu này dành cho các nhà phát triển để hiểu cách hệ thống hoạt động bên dưới.

## 1. Tổng quan Kiến trúc

Ứng dụng tuân theo kiến trúc **Client-Server** tiêu chuẩn:

*   **Client (C++)**: Xử lý tương tác người dùng, mã hóa/giải mã và quản lý khóa. **Quan trọng: Nội dung file chưa mã hóa và Master Key không bao giờ rời khỏi RAM của Client.**
*   **Server (C++)**: Đóng vai trò là nhà cung cấp lưu trữ "Zero-Knowledge" (Không kiến thức). Server lưu trữ dữ liệu đã mã hóa và metadata của người dùng nhưng không thể đọc nội dung file thực tế vì không nắm giữ khóa giải mã.
*   **Giao tiếp**: Sử dụng HTTP REST API với định dạng dữ liệu JSON.

## 2. Tiêu chuẩn Bảo mật

Sử dụng các nguyên tắc mã hóa tiêu chuẩn công nghiệp được cung cấp bởi **OpenSSL**:

| Thành phần | Thuật toán | Mục đích |
| :--- | :--- | :--- |
| **Mã hóa Đối xứng** | **AES-256-CBC** | Mã hóa nội dung file và bao gói khóa (key wrapping). |
| **Hàm băm (Hashing)** | **SHA-256** | Băm mật khẩu (phía server) và kiểm tra tính toàn vẹn. |
| **Dẫn xuất khóa (KDF)** | **PBKDF2** (HMAC-SHA256) | Tạo **Master Key** từ mật khẩu và salt của người dùng. |
| **Trao đổi khóa** | **ECDH** | Chia sẻ khóa file an toàn giữa các người dùng (Diffie-Hellman). |
| **Ngẫu nhiên** | **CSPRNG** (OpenSSL) | Tạo Salt, IV và File Key ngẫu nhiên an toàn. |

## 3. Luồng Xử lý Logic

### 3.1. Đăng ký & Đăng nhập (Xác thực)

Mục tiêu là xác thực người dùng trong khi đảm bảo Server không bao giờ lưu trữ mật khẩu dạng văn bản thuần (plain-text).

1.  **Đăng ký**:
    *   Client tạo cặp khóa **Receive Key** (`receive_pub_key`, `receive_pri_key`).
    *   Client gửi `username`, `password` và `receive_pub_key` lên Server.
    *   Server tạo một chuỗi ngẫu nhiên **Salt** (16 bytes).
    *   Server tính toán `Hash = SHA256(password + salt)`.
    *   Server lưu `(username, Hash, Salt, receive_pub_key)` vào cơ sở dữ liệu SQLite.

2.  **Đăng nhập**:
    *   Client gửi `username` và `password`.
    *   Server tìm người dùng, lấy `Salt`.
    *   Server tính toán `Hash_Check = SHA256(password + Salt)`.
    *   Nếu `Hash_Check` khớp với `Hash` đã lưu:
        *   Server tạo **Session Token** (JWT hoặc chuỗi ngẫu nhiên).
        *   Server trả về `{ success: true, token: "...", salt: "..." }`.

3.  **Dẫn xuất Master Key (Phía Client)**:
    *   Sau khi đăng nhập thành công, Client nhận được **Salt**.
    *   Client tính toán **Master Key** từ **Password** và **Salt**:
        ```cpp
        MasterKey = PBKDF2(Password, Salt, Iterations=10000, KeyLen=32 bytes)
        ```
    *   *Lưu ý*: `MasterKey` này được giữ trong RAM và dùng để mã hóa/giải mã các khóa khác. Nó **không bao giờ** được gửi lên Server.

### 3.2. Tải lên File Bảo mật (Mã hóa)

Sử dụng cơ chế **Bao gói Khóa** (Key Wrapping / Envelope Encryption) để cho phép chia sẻ file trong tương lai.

1.  **Tạo Khóa**:
    *   Client tạo ngẫu nhiên **File Key** (32 bytes) cho *riêng file này*.
    *   Client tạo ngẫu nhiên **IV** (16 bytes).

2.  **Mã hóa File**:
    *   `EncryptedContent = AES_Encrypt(FileContent, FileKey, IV)`

3.  **Bao gói Khóa (Key Wrapping)**:
    *   Client mã hóa `FileKey` bằng `MasterKey` của người dùng.
    *   `WrappedKey = AES_Encrypt(FileKey, MasterKey, IV_Wrap)`

4.  **Tải lên (Upload)**:
    *   Client gửi JSON lên Server:
        ```json
        {
          "encrypted_content": "Base64(...)",
          "wrapped_key": "Base64(...)",
          "iv_hex": "Hex(...)"
        }
        ```
    *   Server lưu blob này vào bảng `Notes`.

### 3.3. Tải xuống File Bảo mật (Giải mã)

1.  **Tải xuống (Download)**:
    *   Client yêu cầu file theo ID.
    *   Server trả về `{ encrypted_content, wrapped_key, iv_hex }`.

2.  **Mở gói Khóa (Unwrapping)**:
    *   Client giải mã `WrappedKey` bằng `MasterKey`.
    *   `FileKey = AES_Decrypt(WrappedKey, MasterKey)`

3.  **Giải mã File**:
    *   Client giải mã nội dung bằng `FileKey` vừa khôi phục.
    *   `OriginalContent = AES_Decrypt(EncryptedContent, FileKey, IV)`

### 3.4. Chia sẻ File Bảo mật (Ephemeral-Static ECDH)

Sử dụng 2 cặp khóa cho quá trình này:
1.  **Receive Key Pair** (`receive_pub_key`, `receive_pri_key`): Khóa để nhận file của người nhận.
2.  **Send Key Pair** (`send_pub_key`, `send_pri_key`): Khóa để gửi file của người gửi.

**Bước 1: Chuẩn bị**
*   **Client B (Người nhận)**: Có cặp khóa `receive_pub_key` và `receive_pri_key`.
*   `receive_pub_key` đã được gửi và lưu trên Server từ khi đăng ký.

**Bước 2: Client A (Người gửi) thực hiện chia sẻ**
1.  **Tạo Send Key**:
    *   Client A sinh một cặp khóa `send_pub_key` và `send_pri_key` mới.
2.  **Tính toán Bí mật chung**:
    *   Lấy `receive_pub_key` của B từ Server.
    *   Tính `S = ECDH(send_pri_key, receive_pub_key)`.
    *   Dẫn xuất **Khóa phiên**: `SessionKey = KDF(S)`.
3.  **Mã hóa khóa file**:
    *   Client A giải mã lấy `FileKey` gốc.
    *   Mã hóa `FileKey` bằng `SessionKey`: `NewWrappedKey = AES_Encrypt(FileKey, SessionKey)`.
4.  **Gửi và Hủy**:
    *   Client A gửi lên Server: `{ note_id, receiver="Bob", send_pub_key=..., new_wrapped_key=NewWrappedKey }`.
    *   **QUAN TRỌNG**: Client A lập tức **xóa vĩnh viễn** `send_pri_key` và `SessionKey`.

**Bước 3: Client B (Người nhận) nhận file**
1.  **Tải thông tin**: Client B tải `NewWrappedKey` và `send_pub_key` của A từ Server.
2.  **Tái tạo Bí mật chung**:
    *   Client B dùng `receive_pri_key` của mình.
    *   Tính `S = ECDH(receive_pri_key, send_pub_key)`.
    *   Dẫn xuất lại **Khóa phiên**: `SessionKey = KDF(S)`.
3.  **Giải mã**:
    *   Dùng `SessionKey` để giải mã `NewWrappedKey` lấy `FileKey`.
    *   Dùng `FileKey` để giải mã nội dung file.

## 4. Cấu trúc Lưu trữ Dữ liệu (Data Storage)

Phần này mô tả chi tiết dữ liệu được lưu trữ ở đâu.

### 4.1. Server lưu trữ những gì? (SQLite DB)
Server lưu trữ dữ liệu bền vững trong cơ sở dữ liệu `secure_notes.db`.

**Bảng `Users` (Thông tin người dùng)**
| Cột | Mô tả |
| :--- | :--- |
| `id` | ID định danh người dùng. |
| `username` | Tên đăng nhập. |
| `password_hash` | Mật khẩu đã băm (SHA256 + Salt). **Không lưu mật khẩu gốc.** |
| `salt` | Chuỗi ngẫu nhiên dùng để băm mật khẩu và tạo Master Key. |
| `receive_public_key_hex` | Khóa công khai (Receive Key) để người khác tìm và gửi file. |

**Bảng `Notes` (Dữ liệu ghi chú)**
| Cột | Mô tả |
| :--- | :--- |
| `id` | ID định danh ghi chú. |
| `user_id` | ID người sở hữu. |
| `encrypted_content` | Nội dung file **đã mã hóa AES**. Server không đọc được. |
| `wrapped_key` | Khóa file **đã mã hóa** (bởi Master Key hoặc Session Key). |
| `iv_hex` | Vector khởi tạo (IV) cần thiết cho giải mã AES. |
| `created_at` | Thời gian tạo. |

### 4.2. Các biến dữ liệu Client (Client Variables)

Dưới đây là danh sách các biến dữ liệu quan trọng mà Client quản lý và xử lý:

| Tên Biến | Kiểu Dữ liệu | Mô tả |
| :--- | :--- | :--- |
| `master_key` | `vector<byte>` | Khóa chính dùng để bao gói các khóa khác. |
| `auth_token` | `string` | Token xác thực (JWT) nhận từ Server. |
| `current_username` | `string` | Tên người dùng hiện tại. |
| `receive_pri_key` | `vector<byte>` | Khóa riêng tư định danh (Identity Key) để nhận file. |
| `receive_pub_key` | `vector<byte>` | Khóa công khai định danh tương ứng. |
| `send_pri_key` | `vector<byte>` | Khóa riêng tư tạm thời (Ephemeral) khi gửi file. |
| `send_pub_key` | `vector<byte>` | Khóa công khai tạm thời tương ứng. |
| `session_key` | `vector<byte>` | Khóa phiên ECDH (Shared Secret). |
| `file_key` | `vector<byte>` | Khóa AES dùng để mã hóa/giải mã nội dung file. |
| `file_content` | `vector<byte>` | Nội dung file gốc (Plain text). |

## 4. Cấu trúc Cơ sở dữ liệu (SQLite)
