// Client/AppLogic.h
#pragma once
#include "network.h"
#include <string>
#include <vector>

class AppLogic {
private:
    Network* net;
    std::vector<unsigned char> master_key; // Key trong RAM
    std::string current_username;
    std::vector<unsigned char> receive_private_key; // Receive Key để nhận file chia sẻ
    std::vector<unsigned char> receive_public_key;

public:
    AppLogic();
    
    // Login: Gửi user/pass -> Nhận Salt -> Tính MasterKey -> Nhận Token
    bool login(std::string user, std::string pass);

    // Register: Đăng ký tài khoản mới
    bool registerUser(std::string user, std::string pass);

    // Upload: Đọc file -> Mã hóa AES -> Mã hóa Key -> Gửi Server
    void uploadFile(std::string filepath);

    // Download: Tải về -> Giải mã Key -> Giải mã File -> Ghi ra đĩa
    void downloadFile(int note_id);

    // List: Liệt kê tất cả ghi chú của user hiện tại
    void listNotes();

    // Delete: Xóa ghi chú theo ID
    void deleteNote(int note_id);

    // Share: Chia sẻ ghi chú với người dùng khác (ECDH)
    // 1. Lấy receive_public_key của người nhận từ Server
    // 2. Tạo cặp khóa send_key tạm thời (ephemeral)
    // 3. Tính session_key = ECDH(send_private_key, receiver_receive_public_key)
    // 4. Giải mã file_key từ wrapped_key bằng master_key
    // 5. Mã hóa file_key bằng session_key -> new_wrapped_key
    // 6. Gửi (note_id, recipient_username, send_public_key, new_wrapped_key, duration) lên Server
    // 7. XÓA send_private_key và session_key ngay lập tức
    void shareNoteWithUser(int note_id, std::string recipient_username, int duration_seconds);

    // Create Share Link: Tạo URL tạm thời chỉ cho các username cụ thể
    // 1. Client nhập danh sách username được phép truy cập
    // 2. Với mỗi username: Lấy receive_public_key, tính ECDH session_key, mã hóa file_key
    // 3. Gửi danh sách {username, send_public_key, wrapped_key} lên Server
    // 4. Server tạo token, lưu token với danh sách user được phép
    // 5. Trả về URL: http://server/share/{token}
    // 6. Khi truy cập URL, phải đăng nhập, server check username có trong whitelist không
    std::string createShareLink(int note_id, std::vector<std::string> allowed_usernames, int duration_seconds);

    // Access Shared Note: Truy cập ghi chú qua URL/token (CẦN ĐĂNG NHẬP)
    // 1. Gửi GET /share/{token} với auth token
    // 2. Server check: token còn hạn? username hiện tại có trong whitelist?
    // 3. Nếu hợp lệ, trả về (encrypted_content, wrapped_key_for_this_user, iv)
    // 4. Client dùng receive_private_key và sender_send_public_key để tính session_key
    // 5. Giải mã wrapped_key bằng session_key -> file_key
    // 6. Giải mã encrypted_content bằng file_key
    void accessSharedNote(std::string share_token);

    // Revoke Share: Hủy chia sẻ (xóa token/chia sẻ)
    void revokeShare(std::string share_token);

    // Receive Shared Note: Nhận ghi chú được chia sẻ từ người khác
    // 1. Tải thông tin chia sẻ từ Server (note_id, send_public_key, new_wrapped_key)
    // 2. Tính session_key = ECDH(receive_private_key, sender_send_public_key)
    // 3. Giải mã new_wrapped_key bằng session_key -> file_key
    // 4. Giải mã nội dung bằng file_key
    void receiveSharedNote(int share_id);
};