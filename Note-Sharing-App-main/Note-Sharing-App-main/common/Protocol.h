#pragma once
#include <string>
#include <vector>
#include "../vendor/json.hpp"

// Dùng để trao đổi dữ liệu JSON
using json = nlohmann::json;

// Cấu trúc gói tin Đăng ký (Client -> Server)
struct AuthRequest {
    std::string username;
    std::string password; // Plain text từ client gửi lên
    std::string receivePublicKey; // Khóa công khai nhận (Receive Key)
};
// KHAI BÁO: to_json cho AuthRequest
void to_json(json& j, const AuthRequest& p);

// Cấu trúc gói tin Phản hồi Đăng nhập (Server -> Client)
struct LoginResponse {
    bool success;
    std::string token;   // JWT Token dùng cho các request sau
    std::string salt;    // Salt của user (để Client tính lại Master Key)
    std::string receivePrivateKey; // Private Key của người dùng (Đã được mã hóa/lưu an toàn trên server)
    std::string receivePublicKey; // Public Key của người dùng
    std::string message; // Thông báo lỗi nếu có
};
void from_json(const json& j, LoginResponse& p);

// Cấu trúc gói tin Dữ liệu Ghi chú (dùng cho Upload/Download)
struct NotePayload {
    std::string encryptedContent; // Nội dung ghi chú đã mã hóa
    std::string wrappedKey;       // Key file đã bị mã hóa bởi MasterKey
    std::string iv;               // Vector khởi tạo (Base64)
    std::string metadata;         // Metadata/Tên file
};
void from_json(const json& j, NotePayload& p);
void to_json(json& j, const NotePayload& p);

// Cấu trúc Yêu cầu chia sẻ (Client -> Server)
struct NoteShareRequest {
    int noteId;
    std::string recipientUsername;
    std::string sendPublicKey;
    std::string newWrappedKey; // File Key đã được mã hóa bằng Session Key
    int durationSeconds;                // Thời gian chia sẻ (giây)
};
void to_json(json& j, const NoteShareRequest& p); // <-- KHAI BÁO

// Cấu trúc Entry cho Share Link Whitelist
struct UserAccessItem {
    std::string username;
    std::string sendPublicKey;
    std::string wrappedKey;
};
void to_json(json& j, const UserAccessItem& p); // <-- KHAI BÁO

// Cấu trúc Yêu cầu tạo Link Chia sẻ (Client -> Server)
struct ShareLinkRequest {
    int noteId;
    std::vector<UserAccessItem> userAccessList;
    int durationSeconds;
};
void to_json(json& j, const ShareLinkRequest& p); // <-- KHAI BÁO

// Cấu trúc Phản hồi khi truy cập Share Link (Server -> Client)
struct SharedNoteAccessResponse {
    std::string encryptedContent;
    std::string sendPublicKey;
    std::string wrappedKey;
    std::string iv;
    std::string metadata; // Thêm metadata để đầy đủ thông tin
};
void from_json(const json& j, SharedNoteAccessResponse& p);

// Cấu trúc Phản hồi khi nhận Note trực tiếp qua User-to-User Share
struct SharedNoteReceiveResponse {
    int noteId;
    std::string sendPublicKey;
    std::string newWrappedKey;
    std::string encryptedContent;
    std::string iv;
    std::string metadata; // Thêm metadata để đầy đủ thông tin
};
void from_json(const json& j, SharedNoteReceiveResponse& p);

// Cấu trúc Phản hồi danh sách ghi chú
struct NoteMetadata {
    int noteId;
    std::string createdAt;
    std::string metadata;
};
void from_json(const json& j, NoteMetadata& p);