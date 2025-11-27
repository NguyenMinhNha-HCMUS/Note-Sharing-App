#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>
#include "../common/Protocol.h"

// Struct ánh xạ dữ liệu từ bảng Users
struct UserRecord {
    int id;
    std::string username;
    std::string password_hash; // Đã băm
    std::string salt;
    std::string receive_public_key_hex; // Key ECDH công khai (Receive Key)
};



class Database {
private:
    sqlite3* db; // Con trỏ kết nối DB

public:
    Database();
    ~Database();

    // Khởi tạo bảng (Users, Notes, SharedLinks)
    bool init();

    // --- User Operations ---
    bool createUser(std::string username, std::string pass_hash, std::string salt, std::string receive_pub_key);
    UserRecord getUserByUsername(std::string username);
    bool updateUserPublicKey(int user_id, std::string receive_pub_key);

    // --- Note Operations ---
    // Trả về note_id vừa tạo
    int saveNote(int user_id, std::string encrypted_content, std::string wrapped_key, std::string iv_hex);
    NoteData getNoteById(int note_id);
    
    // --- Sharing Operations ---
    // Tạo link chia sẻ với whitelist username, trả về token chuỗi
    struct UserAccessEntry {
        std::string username;
        std::string send_public_key_hex;
        std::string wrapped_key;
    };
    std::string createShareLink(int note_id, int user_id, 
                                std::vector<UserAccessEntry> user_access_list,
                                int duration_seconds);
    // Kiểm tra quyền truy cập và lấy dữ liệu
    struct ShareLinkData {
        int note_id;
        std::string encrypted_content;
        std::string send_public_key_hex;
        std::string wrapped_key;
        std::string iv_hex;
        bool valid;
    };
    ShareLinkData getShareLinkData(std::string token, std::string username);
    // Xóa link chia sẻ
    bool deleteShareLink(std::string token, int user_id);
    
    // Chia sẻ ghi chú với người dùng cụ thể (User-to-User)
    bool createUserShare(int note_id, int sender_id, int recipient_id, 
                         std::string send_public_key_hex, std::string new_wrapped_key, 
                         int duration_seconds);
    // Lấy danh sách ghi chú được chia sẻ cho user
    std::vector<int> getSharedNotesForUser(int user_id);
    // Lấy thông tin chia sẻ cụ thể
    struct ShareInfo {
        int note_id;
        std::string send_public_key_hex;
        std::string new_wrapped_key;
        std::string encrypted_content;
        std::string iv_hex;
    };
    ShareInfo getShareInfo(int share_id, int recipient_id);
    
    // Lấy danh sách ghi chú của user
    std::vector<NoteData> getNotesForUser(int user_id);
    // Xóa ghi chú
    bool deleteNote(int note_id, int user_id);
};
