# Test Suite cho Secure Note App Server

Thư mục này chứa các file kiểm thử cho server của ứng dụng Secure Note Sharing.

## Cấu trúc

- `test_framework.h` - Framework kiểm thử đơn giản (không cần dependencies bên ngoài)
- `Database.test.cpp` - Kiểm thử các chức năng Database
- `Auth.test.cpp` - Kiểm thử xác thực và token
- `Crypto.test.cpp` - Kiểm thử các hàm mã hóa
- `main.cpp` - File chạy tất cả các test
- `build_tests.sh` - Script build tests

## Cách sử dụng

### Build tests

```bash
cd tests
chmod +x build_tests.sh
./build_tests.sh
```

### Chạy tests

```bash
cd tests
./test_runner
```

## Các test cases

### Database Tests
- `Database_Init` - Kiểm tra khởi tạo database
- `Database_CreateUser` - Tạo user mới
- `Database_CreateUser_Duplicate` - Kiểm tra không cho phép tạo user trùng
- `Database_GetUserByUsername` - Lấy thông tin user theo username
- `Database_GetUserByUsername_NotFound` - Xử lý khi không tìm thấy user
- `Database_SaveNote` - Lưu ghi chú
- `Database_GetNoteById` - Lấy ghi chú theo ID
- `Database_GetNotesForUser` - Lấy danh sách ghi chú của user
- `Database_DeleteNote` - Xóa ghi chú
- `Database_CreateUserShare` - Chia sẻ ghi chú giữa users
- `Database_CreateShareLink` - Tạo link chia sẻ

### Auth Tests
- `Auth_GenerateToken` - Tạo token
- `Auth_VerifyToken_Valid` - Xác thực token hợp lệ
- `Auth_VerifyToken_Invalid` - Xử lý token không hợp lệ
- `Auth_VerifyToken_Empty` - Xử lý token rỗng
- `Auth_VerifyToken_Modified` - Xử lý token bị sửa đổi
- `Auth_ExtractToken_Bearer` - Trích xuất token từ header Bearer
- `Auth_ExtractToken_NoBearer` - Trích xuất token không có Bearer prefix
- `Auth_TokenExpiry` - Kiểm tra thời gian hết hạn token
- `Auth_DifferentUsers` - Token của các user khác nhau

### Crypto Tests
- `Crypto_GenerateRandomBytes` - Tạo bytes ngẫu nhiên
- `Crypto_HashSHA256` - Hash SHA256
- `Crypto_ToHex` / `Crypto_FromHex` - Chuyển đổi hex
- `Crypto_Base64EncodeDecode` - Mã hóa/giải mã Base64
- `Crypto_AES_EncryptDecrypt` - Mã hóa/giải mã AES
- `Crypto_AES_WrongKey` - Xử lý khi dùng sai khóa

## Lưu ý

- Tests sử dụng database file riêng: `test_secure_notes.db`
- Database test file sẽ được tự động xóa sau mỗi test
- Đảm bảo đã build server trước khi chạy tests (để có các dependencies)

## Thêm test mới

Để thêm test mới, sử dụng macro `TEST`:

```cpp
TEST(MyNewTest) {
    // Your test code here
    ASSERT_TRUE(condition);
    ASSERT_EQ(a, b);
    return true;
}
```

Các macro assertion có sẵn:
- `ASSERT_TRUE(condition)` - Kiểm tra điều kiện đúng
- `ASSERT_FALSE(condition)` - Kiểm tra điều kiện sai
- `ASSERT_EQ(a, b)` - Kiểm tra bằng nhau
- `ASSERT_NE(a, b)` - Kiểm tra khác nhau
- `ASSERT_STREQ(a, b)` - Kiểm tra chuỗi bằng nhau

