## 1. Mục tiêu & phạm vi

Server được cài đặt để cung cấp API chia sẻ ghi chú bảo mật theo mô hình **zero‑knowledge**:

- **Mục tiêu chính**:
  - Xác thực người dùng an toàn, không lưu mật khẩu dạng plain‑text.
  - Lưu trữ ghi chú **đã mã hóa**, server không thể đọc nội dung.
  - Hỗ trợ chia sẻ ghi chú: trực tiếp user‑to‑user và qua share link có whitelist + thời gian hết hạn.
- **Phạm vi**: Mô tả phần cài đặt phía server (C++): kiến trúc, các module chính, trick/optimize, thách thức & kiểm thử.

## 2. Kiến trúc & công nghệ sử dụng

- **Kiến trúc**:
  - Mô hình **Client–Server** qua HTTP REST.
  - Định dạng trao đổi dữ liệu: **JSON**.
  - Server stateless ở mức session (token tự chứa thông tin user + expiry).
- **Công nghệ / thư viện**:
  - **Crow** (`crow_all.h`): web framework C++ dùng để định nghĩa REST API (`CROW_ROUTE`, `crow::SimpleApp`).
  - **nlohmann/json** (`json.hpp`): parse / serialize JSON request & response.
  - **SQLite3** (`sqlite3.c/.h`): lưu trữ bền vững Users, Notes, SharedLinks, UserShares.
  - **OpenSSL**: SHA‑256, AES‑256‑CBC, PBKDF2, ECDH, base64.
  - **Boost / Asio**: hỗ trợ networking cho Crow.
  - **Module nội bộ**:
    - `Database`: trừu tượng hóa toàn bộ truy vấn SQLite.
    - `Auth`: sinh & verify token, trích xuất token từ header.
    - `Crypto`: hàm mã hóa, hash, random, hex/base64, ECDH.
    - `Protocol`: struct dữ liệu trao đổi (NoteData, AuthRequest, v.v.).

## 3. Chi tiết cài đặt & các trick/optimize

### 3.1. Lớp `Database` & quản lý SQLite

- **Khởi tạo CSDL**:
  - Constructor `Database::Database()` mở file `secure_notes.db` bằng `sqlite3_open`.
  - Hàm `init()` tạo đầy đủ 5 bảng nếu chưa tồn tại:
    - `Users`, `Notes`, `SharedLinks`, `SharedLinkAccess`, `UserShares`.
- **Thiết kế bảng**:
  - Dùng `INTEGER PRIMARY KEY AUTOINCREMENT` cho tất cả ID để truy xuất và join đơn giản.
  - Lưu **timestamp** (unix time, `INTEGER`) cho `created_at`, `expiration_time` → so sánh thời gian nhanh, không phụ thuộc múi giờ.
  - Ràng buộc **FOREIGN KEY** giữa Users–Notes–Shares giúp đảm bảo toàn vẹn dữ liệu.
- **API thao tác dữ liệu chính**:
  - User:
    - `createUser(username, pass_hash, salt, receive_pub_key)`.
    - `getUserByUsername(username)`.
    - `updateUserPublicKey(...)`.
  - Notes:
    - `saveNote(user_id, encrypted_content, wrapped_key, iv_hex)` trả về `note_id`.
    - `getNoteById(note_id)`.
    - `getNotesForUser(user_id)` trả về danh sách `NoteData`.
    - `deleteNote(note_id, user_id)` kiểm tra sở hữu trước khi xóa + dọn dẹp các bản ghi share liên quan.
  - Sharing:
    - Share trực tiếp: `createUserShare`, `getSharedNotesForUser`, `getShareInfo`.
    - Share link: `createShareLink`, `getShareLinkData`, `deleteShareLink`.
- **Trick / tối ưu**:
  - **Chuẩn bị statement**: mọi truy vấn ghi/đọc đều dùng `sqlite3_prepare_v2` + `sqlite3_bind_*` → tránh SQL injection, tái sử dụng plan của SQLite.
  - **Kiểm tra ownership bằng SQL**:
    - Ví dụ `deleteNote` trước hết chạy `SELECT id FROM Notes WHERE id = ? AND user_id = ?` để đảm bảo user chỉ xóa note của chính mình.
  - **Quản lý expiry ngay trong query**:
    - `getSharedNotesForUser` và `getShareInfo` đều filter `expiration_time > now`, giảm logic kiểm tra ở app layer.

### 3.2. Lớp `Auth` – xác thực & token

- **Thiết kế token**:
  - Dạng chuỗi: `base64(user_id:username:exp).signature`.
  - `exp`: thời điểm hết hạn tính bằng unix timestamp (giây).
  - `signature = SHA256(SERVER_SECRET + base64_part)`.
- **Sinh token (`Auth::generateToken`)**:
  - Tính `exp = now + TOKEN_TTL_SECONDS` (1 giờ).
  - Nối `user_id:username:exp`, base64 encode để tránh ký tự đặc biệt.
  - Hash SHA‑256 với `SERVER_SECRET` để tạo chữ ký.
- **Xác thực (`Auth::verifyToken`)**:
  - Tách token theo dấu `.` lấy base và signature.
  - Tính lại `expectedSig = SHA256(SERVER_SECRET + base)`:
    - Nếu khác `providedSig` → token bị sửa → invalid.
  - Giải mã base64, parse lại `user_id`, `username`, `exp`.
  - So sánh `exp` với `now`:
    - Nếu token hết hạn → `valid = false`.
    - Ngược lại trả về `TokenPayload{user_id, username, valid=true, exp}`.
- **Trích xuất từ header (`extractToken`)**:
  - Hỗ trợ format chuẩn: `Authorization: Bearer <token>`.
  - Nếu không có prefix, coi cả header là token (tiện cho test/manual call).

**Trick / lý do thiết kế**:

- Không dùng JWT full stack để giữ cài đặt nhẹ, dễ kiểm soát, nhưng vẫn đảm bảo:
  - Có chữ ký HMAC‑like với secret.
  - Có expiry bên trong token, không cần lưu session trên DB.

### 3.3. Lớp `Crypto` – mã hóa & tiện ích

- **Hash & KDF**:
  - `hashSHA256(input)`: dùng `SHA256` của OpenSSL để băm chuỗi.
  - `deriveKeyPBKDF2(password, salt)`: dùng PBKDF2‑HMAC‑SHA256 để sinh **Master Key** phía client (server chỉ cần cùng hàm để debug nếu cần).
- **Random & encoding**:
  - `generateRandomBytes(length)`: dùng `RAND_bytes` → CSPRNG đạt chuẩn cryptographic.
  - `toHex` / `fromHex`: biểu diễn bytes thành hex dùng cho `salt`, `iv_hex`, public key.
  - `base64Encode` / `base64Decode`: dùng BIO của OpenSSL, flag `BIO_FLAGS_BASE64_NO_NL` để không xuống dòng.
- **Mã hóa đối xứng**:
  - `encryptAES(plaintext, key, iv)` và `decryptAES(ciphertext, key, iv)` sử dụng `EVP_CIPHER_CTX` với `EVP_aes_256_cbc()`.
  - Dùng **File Key** + IV riêng cho từng ghi chú; `wrapped_key` chính là File Key đã được mã hóa bởi Master Key / Session Key (ECDH).
- **ECDH**:
  - `generateECDHKeyPair()` sinh cặp khóa trên curve chuẩn (OpenSSL EC).
  - `computeECDHSecret(my_private_key_hex, peer_public_key_hex)` tính shared secret để tạo Session Key khi share file.

**Trick / optimize**:

- Tất cả hàm Crypto đều tách biệt, thuần dữ liệu (không phụ thuộc HTTP/DB) → dễ unit test, dễ tái sử dụng sang client.
- Sử dụng hex & base64 để các khóa / ciphertext có thể truyền an toàn qua JSON.

### 3.4. Cài đặt REST API trong `server_main.cpp`

- **Khởi tạo**:
  - Tạo `Database db; db.init();`.
  - Tạo `crow::SimpleApp app;` và khai báo routes thông qua `CROW_ROUTE`.

- **Nhóm Auth**:
  - `POST /register`:
    - Parse JSON: `username`, `password`, `receive_public_key_hex`.
    - Check user trùng bằng `db.getUserByUsername`.
    - Sinh `salt = Crypto::generateRandomBytes(16)`, hash mật khẩu bằng `Crypto::hashSHA256(password + salt)`.
    - Ghi vào bảng `Users` qua `db.createUser`.
  - `POST /login`:
    - Lấy user từ DB, hash lại `password + salt`, so sánh.
    - Nếu đúng, sinh token qua `Auth::generateToken` và trả về `{ token, salt }`.

- **Nhóm Notes**:
  - `POST /upload`:
    - Đọc header `Authorization`, lấy token bằng `Auth::extractToken` → verify.
    - Parse `encrypted_content`, `wrapped_key`, `iv_hex`.
    - Lưu vào bảng `Notes` với `user_id` từ token.
  - `GET /notes`:
    - Verify token → `auth.user_id`.
    - Lấy danh sách ghi chú user qua `db.getNotesForUser` và trả về mảng JSON `{ note_id, created_at }`.
  - `GET /note/<id>`:
    - Verify token.
    - Lấy note bằng `getNoteById`.
    - Kiểm tra quyền sở hữu bằng cách xem note có nằm trong `getNotesForUser(auth.user_id)` hay không → tránh user đọc note của người khác.
  - `DELETE /note/<id>`:
    - Verify token.
    - Gọi `db.deleteNote(note_id, auth.user_id)`:
      - Vừa kiểm tra quyền, vừa xóa cascade các share liên quan.

- **Nhóm Share trực tiếp (user‑to‑user)**:
  - `POST /share`:
    - Verify token (người gửi).
    - Body chứa `note_id`, `recipient_username`, `send_public_key_hex`, `new_wrapped_key`, `duration_seconds`.
    - Tìm recipient trong bảng `Users`, lưu bản ghi vào `UserShares`.
  - `GET /shared`:
    - Lấy danh sách `share_id` còn hiệu lực cho user hiện tại bằng `getSharedNotesForUser`.
  - `GET /shared/<id>`:
    - Trả về `note_id`, `send_public_key_hex`, `new_wrapped_key`, `encrypted_content`, `iv_hex` từ bảng `UserShares` join `Notes`.

- **Nhóm Share link (whitelist)**:
  - `POST /share/link`:
    - Verify token.
    - Nhận `note_id`, `duration_seconds`, và `user_access_list` (username + send_public_key_hex + wrapped_key).
    - Gọi `db.createShareLink`:
      - Sinh token chuỗi ngẫu nhiên bằng `Crypto::generateRandomBytes(32)` + hex.
      - Ghi vào bảng `SharedLinks` và `SharedLinkAccess`.
    - Trả về URL share + token + thời gian hết hạn.
  - `GET /share/<token>`:
    - Yêu cầu user đang đăng nhập (phải có token auth).
    - `db.getShareLinkData(token, auth.username)`:
      - Kiểm tra link tồn tại, chưa hết hạn, user có trong whitelist.
      - Lấy `encrypted_content`, `wrapped_key`, `send_public_key_hex`, `iv_hex`.
  - `DELETE /share/<token>`:
    - Verify token của owner.
    - `db.deleteShareLink` xóa cả `SharedLinks` và `SharedLinkAccess`.

**Các điểm bảo mật chính**:

- Mọi API nhạy cảm (upload note, đọc note, chia sẻ, lấy share) đều:
  - Bắt buộc header `Authorization`.
  - Gọi `Auth::verifyToken` và dừng ngay với HTTP 401 nếu invalid.
- Các thao tác trên ghi chú luôn gắn với `auth.user_id` hoặc kiểm tra quyền trong DB trước khi trả dữ liệu.

## 4. Thách thức & giải pháp

- **Thiết kế token nhẹ nhưng an toàn**:
  - *Thách thức*: Không muốn kéo thêm dependency JWT phức tạp nhưng vẫn cần expiry + chống sửa nội dung.
  - *Giải pháp*: Tự cài đặt định dạng `base64(payload).HMAC` với SHA‑256 và secret server; verify chữ ký và expiry mỗi request.
- **Mô hình chia sẻ đa dạng (user‑to‑user + share link)**:
  - *Thách thức*: Tránh trùng lặp logic lưu trữ và rối schema khi vừa phải support whitelist, vừa phải support share trực tiếp.
  - *Giải pháp*: Tách rõ 2 bảng:
    - `UserShares` cho chia sẻ trực tiếp (khóa theo `recipient_id`).
    - `SharedLinks` + `SharedLinkAccess` cho link + whitelist nhiều user.
- **Đảm bảo zero‑knowledge phía server**:
  - *Thách thức*: Không để logic server vô tình đòi hỏi plaintext hoặc Master Key.
  - *Giải pháp*: Tất cả API chỉ làm việc với:
    - `encrypted_content`, `wrapped_key`, `iv_hex`, public key.
    - Không có API nào nhận/lưu plaintext note hay Master Key.

## 5. Phương pháp, công cụ kiểm thử & kết quả

### 5.1. Phương pháp kiểm thử

- **Unit test theo module**:
  - `Database`: kiểm tra tạo user, ghi/đọc/xóa note, chia sẻ trực tiếp, tạo share link.
  - `Auth`: sinh token, verify hợp lệ, xử lý token invalid/expired/sửa nội dung.
  - `Crypto`: random, hash, hex/base64, AES encrypt/decrypt với key đúng/sai.
- **Chiến lược**:
  - Mỗi API/hàm quan trọng có tối thiểu 1–2 test bao trùm happy path + lỗi phổ biến (user trùng, token sai, không có quyền truy cập, v.v.).

### 5.2. Công cụ / framework kiểm thử

- **Framework tự xây** (`tests/test_framework.h`):
  - Cung cấp macro `TEST(name)` để khai báo test case, tự đăng ký vào `TestFramework`.
  - Các macro assert: `ASSERT_TRUE`, `ASSERT_FALSE`, `ASSERT_EQ`, `ASSERT_NE`, `ASSERT_STREQ`.
  - Hàm `runAll()` in kết quả từng test và tổng kết số pass/fail.
- **Tổ chức test**:
  - `Database.test.cpp`: ~11 test cases cho CSDL.
  - `Auth.test.cpp`: ~9 test cases cho token & xác thực.
  - `Crypto.test.cpp`: ~8 test cases cho mã hóa/hàm tiện ích.
  - `main.cpp`: chạy toàn bộ tests.
- **Build & chạy tests**:
  - Script `tests/build_tests.sh`:
    - Dò OpenSSL, build `sqlite3.o`, rồi biên dịch `test_runner` với `Database.cpp`, `Auth.cpp`, `Crypto.cpp`.
  - Chạy:
    - `./tests/build_tests.sh`
    - `./tests/test_runner`

### 5.3. Kết quả kiểm thử

- Kết quả chạy gần nhất:

```text
==========================================
    TEST RESULTS
==========================================
Passed: 29
Failed: 0
Total:  29
==========================================
```

- **Database**:
  - Kiểm tra khởi tạo, tạo user mới/duplicate, lấy user theo username, lưu/lấy/xóa note, chia sẻ giữa users, tạo share link, lọc theo thời gian hết hạn.
- **Auth**:
  - Token sinh ra không rỗng, parse đúng `user_id`/`username`/`exp`, phát hiện token sai format, sửa chữ ký, token hết hạn.
- **Crypto**:
  - Random bytes thực sự thay đổi, SHA‑256 cho cùng input cho cùng hash, hex/base64 encode/decode đảo ngược được, AES encrypt/decrypt trả lại đúng plaintext, decrypt với wrong key không khớp dữ liệu gốc.

## 6. Đánh giá & hướng phát triển

- **Đánh giá hiện tại**:
  - Server đã đáp ứng các yêu cầu:
    - Lưu trữ zero‑knowledge: chỉ xử lý dữ liệu đã mã hóa và metadata.
    - Xác thực token‑based đơn giản, dễ tích hợp client C++.
    - Hỗ trợ đủ các luồng: đăng ký, đăng nhập, upload, xem/xóa ghi chú, chia sẻ trực tiếp, share link whitelist.
  - Hệ thống kiểm thử tự động giúp đảm bảo Database/Auth/Crypto hoạt động đúng sau mỗi lần build.
- **Hướng mở rộng**:
  - Bổ sung logging bảo mật (ẩn thông tin nhạy cảm).
  - Tách SERVER_SECRET ra file cấu hình / biến môi trường.
  - Thêm integration test ở mức HTTP (dùng curl/script) để kiểm tra full flow end‑to‑end.


