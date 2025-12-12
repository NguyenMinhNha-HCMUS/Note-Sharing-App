#pragma once
#include <string>
#include <vector>
#include "../vendor/json.hpp"

// Dùng để trao đổi dữ liệu JSON

// Cấu trúc gói tin Đăng ký (Client -> Server)
struct AuthRequest {
    std::string username;
    std::string password; // Plain text từ client gửi lên
    std::string receive_public_key_hex; // Khóa công khai nhận (Receive Key)
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AuthRequest, username, password, receive_public_key_hex)

// Cấu trúc gói tin Phản hồi Đăng nhập (Server -> Client)
struct LoginResponse {
    bool success;
    std::string token;   // JWT Token dùng cho các request sau
    std::string salt;    // Salt của user (để Client tính lại Master Key)
    std::string message = ""; // Thông báo lỗi nếu cần

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginResponse, success, token, salt)
};

// Cấu trúc dữ liệu của một Ghi chú khi gửi qua mạng
struct NoteData {
    int note_id;
    std::string encrypted_content; // Nội dung ghi chú đã mã hóa
    std::string wrapped_key;       // Key file đã bị mã hóa bởi MasterKey
    std::string iv_hex;            // Vector khởi tạo
    std::string filename;          // Tên file gốc
    long created_at;               // Timestamp tạo ghi chú
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NoteData, note_id, encrypted_content, wrapped_key, iv_hex, filename, created_at)

// Cấu trúc yêu cầu chia sẻ ghi chú (Client -> Server)
struct NoteShareRequest {
    int note_id;                        // ID của ghi chú cần chia sẻ
    std::string recipient_username;     // Tên người dùng nhận chia sẻ
    std::string send_public_key_hex;    // Khóa công khai tạm thời (Send Key) của người gửi
    std::string new_wrapped_key;        // File Key đã được mã hóa bằng Session Key
    int duration_time;                  // Thời gian chia sẻ
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NoteShareRequest, note_id, recipient_username, send_public_key_hex, new_wrapped_key, duration_time)

// Cấu trúc phản hồi chia sẻ ghi chú (Server -> Client)
struct NoteShareResponse {
    std::string share_link;  // Link chia sẻ ghi chú
    long expiration_at;      // Thời điểm hết hạn chia sẻ (timestamp)
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NoteShareResponse, share_link, expiration_at)

// Gói tin chứa Public Key (Dùng cho E2EE - Diffie Hellman)
struct PublicKeyPayload {
    std::string username;
    std::string receive_public_key_hex; // Khóa công khai nhận (Receive Key)
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PublicKeyPayload, username, receive_public_key_hex)

// Cấu trúc phản hồi danh sách ghi chú
struct NoteListItem {
    int note_id;
    long created_at;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NoteListItem, note_id, created_at)

// Cấu trúc entry trong danh sách truy cập share link
struct ShareLinkUserAccess {
    std::string username;
    std::string send_public_key_hex;
    std::string wrapped_key;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShareLinkUserAccess, username, send_public_key_hex, wrapped_key)

// Cấu trúc yêu cầu tạo link chia sẻ với whitelist
struct CreateShareLinkRequest {
    int note_id;
    std::vector<ShareLinkUserAccess> user_access_list;
    int duration_seconds;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CreateShareLinkRequest, note_id, user_access_list, duration_seconds)

// Cấu trúc phản hồi khi truy cập share link
struct ShareLinkAccessResponse {
    std::string encrypted_content;
    std::string send_public_key_hex;
    std::string wrapped_key;
    std::string iv_hex;
    std::string filename;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ShareLinkAccessResponse, encrypted_content, send_public_key_hex, wrapped_key, iv_hex, filename)

// Cấu trúc thông tin chia sẻ user-to-user
struct UserShareInfo {
    int share_id;
    int note_id;
    std::string sender_username;
    std::string send_public_key_hex;
    std::string new_wrapped_key;
    std::string encrypted_content;
    std::string iv_hex;
    long expiration_at;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserShareInfo, share_id, note_id, sender_username, send_public_key_hex, new_wrapped_key, encrypted_content, iv_hex, expiration_at);