#include "client_app_logic.h"
#include "../common/Crypto.h"
#include "../common/Protocol.h"
#include <fstream>
#include <iostream>
#include "../vendor/json.hpp"

using json = nlohmann::json;

AppLogic::AppLogic() {
    net = new Network("http://localhost:8080");
}

// --------------------------------------------------------
// MARK: - Authentication
// --------------------------------------------------------

bool AppLogic::registerUser(std::string user, std::string pass) {
    // Requirement:
    // 1. Generate ECDH Key Pair (Receive Key).
    DHKeyPair keyPair = Crypto::generateECDHKeyPair();
    if (keyPair.publicKey.empty()) {
        std::cerr << "[ERROR] Khong tao duoc ECDH key pair.\n";
        return false;
    }

    // 2. Create AuthRequest with username, password, and receive public key.
    AuthRequest req;
    req.username = user;
    req.password = pass; // Plain text (server sẽ hash + salt)
    req.receive_public_key_hex = keyPair.publicKey;

    json j = req;
    std::string json_body = j.dump();

    // 3. Send POST /register to Server.
    std::string response = net->post("/register", json_body);

    // 4. Return true if Server responds 200 OK.
    try {
        json j_resp = json::parse(response);
        if (j_resp.contains("success") && j_resp["success"].get<bool>()) {
            std::cout << "[INFO] Dang ky thanh cong!\n";
            
            // Lưu receive keys vào RAM
            receive_private_key = Crypto::fromHex(keyPair.privateKey);
            receive_public_key = Crypto::fromHex(keyPair.publicKey);
            current_username = user;
            
            // Lưu receive_private_key vào file (encrypted bằng password-derived key)
            // Derive key từ password để mã hóa receive_private_key
            std::string user_salt = Crypto::toHex(Crypto::generateRandomBytes(16));
            std::vector<unsigned char> storage_key = Crypto::deriveKeyPBKDF2(pass, user_salt);
            std::string wrapped_receive_key = Crypto::wrapKey(receive_private_key, storage_key);
            
            // Lưu vào file: salt + wrapped_key
            std::string keyfile_path = "keys/" + user + "_receive.key";
            std::ofstream keyfile(keyfile_path, std::ios::binary);
            if (keyfile.is_open()) {
                keyfile << user_salt << "\n" << wrapped_receive_key;
                keyfile.close();
                std::cout << "[INFO] Luu Receive Private Key vao: " << keyfile_path << "\n";
            } else {
                std::cerr << "[WARNING] Khong the luu Receive Key vao file. Chi ton tai trong RAM!\n";
            }
            
            return true;
        } else {
            std::cerr << "[ERROR] Dang ky that bai: " << j_resp.value("message", "Loi khong xac dinh") << "\n";
            return false;
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
        return false;
    }
}

bool AppLogic::login(std::string user, std::string pass) {
    // Requirement:
    // 1. Send POST /login with username and password.
    AuthRequest req;
    req.username = user;
    req.password = pass; 
    // Không cần gửi public key trong login, chỉ cần user/pass để server trả về info
    req.receive_public_key_hex = ""; 

    json j = req;
    std::string json_body = j.dump();

    std::string response = net->post("/login", json_body);

    try {
        LoginResponse resp;
        json j_resp = json::parse(response);
        // Kiểm tra lỗi HTTP trước khi parse LoginResponse
        if (!j_resp.contains("success")) {
             std::cerr << "[ERROR] Dang nhap that bai: " << j_resp.value("message", "Loi ket noi hoac thong tin server") << "\n";
             return false;
        }

        resp = j_resp.get<LoginResponse>();

        // 2. If success, parse LoginResponse to get Token and Salt.
        if (resp.success) {
            // 3. Store Token.
            net->setToken(resp.token);
            current_username = user;

            // 4. Derive Master Key from Password and Salt using PBKDF2.
            master_key = Crypto::deriveKeyPBKDF2(pass, resp.salt);
            
            if (master_key.empty() || master_key.size() != 32) {
                 std::cerr << "[ERROR] Tao Master Key that bai. Hay kiem tra lai Crypto::deriveKeyPBKDF2.\n";
                 // Xóa token nếu master key không tạo được
                 net->setToken(""); 
                 return false;
            }
            
            // 5. Load receive_private_key từ file encrypted
            std::string keyfile_path = "keys/" + user + "_receive.key";
            std::ifstream keyfile(keyfile_path, std::ios::binary);
            if (keyfile.is_open()) {
                std::string user_salt, wrapped_key;
                std::getline(keyfile, user_salt);
                std::getline(keyfile, wrapped_key);
                keyfile.close();
                
                // Derive storage_key từ password
                std::vector<unsigned char> storage_key = Crypto::deriveKeyPBKDF2(pass, user_salt);
                
                // Unwrap receive_private_key
                receive_private_key = Crypto::unwrapKey(wrapped_key, storage_key);
                
                if (receive_private_key.empty()) {
                    std::cerr << "[WARNING] Khong the giai ma Receive Private Key. Khong the nhan file chia se!\n";
                } else {
                    std::cout << "[INFO] Da load Receive Private Key tu file.\n";
                    
                    // Load receive_public_key từ server
                    std::string pubkey_response = net->get("/user/" + user + "/pubkey");
                    try {
                        json pk_resp = json::parse(pubkey_response);
                        if (pk_resp.contains("receive_public_key_hex")) {
                            receive_public_key = Crypto::fromHex(pk_resp["receive_public_key_hex"].get<std::string>());
                        }
                    } catch (...) {
                        std::cerr << "[WARNING] Khong the load Receive Public Key tu server.\n";
                    }
                }
            } else {
                std::cerr << "[WARNING] Khong tim thay file Receive Key. Khong the nhan file chia se!\n";
            }
            
            std::cout << "[INFO] Dang nhap thanh cong. Token: " << resp.token.substr(0, 10) << "...\n";
            return true;
        } else {
            std::cerr << "[ERROR] Dang nhap that bai: " << resp.message << "\n";
            return false;
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "[ERROR] Loi khong xac dinh khi dang nhap.\n";
        return false;
    }
}

// --------------------------------------------------------
// MARK: - Note Management
// --------------------------------------------------------

void AppLogic::uploadFile(std::string filepath) {
    if (master_key.empty()) {
        std::cerr << "[ERROR] Chua dang nhap hoac Master Key khong ton tai. Hay dang nhap lai.\n";
        return;
    }

    // 1. Read file content from disk (filepath already includes "upload/" prefix).
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Khong mo duoc file: " << filepath << "\n";
        return;
    }
    std::vector<unsigned char> plaintext((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (plaintext.empty()) {
        std::cerr << "[ERROR] File rong hoac doc that bai: " << filepath << "\n";
        return;
    }

    // 2. Generate random File Key and IV.
    std::vector<unsigned char> file_key = Crypto::generateRandomBytes(32); // AES-256
    std::vector<unsigned char> iv = Crypto::generateRandomBytes(16);     // AES-CBC IV

    if (file_key.empty() || iv.empty()) {
        std::cerr << "[ERROR] Khong tao duoc File Key hoac IV.\n";
        return;
    }

    // 3. Encrypt content with File Key (AES-256-CBC).
    std::vector<unsigned char> encrypted_content_bytes = Crypto::encryptAES(plaintext, file_key, iv);
    std::string encrypted_content_b64 = Crypto::base64Encode(encrypted_content_bytes);
    std::string iv_b64 = Crypto::base64Encode(iv);

    // Xóa plaintext khỏi RAM ngay lập tức
    std::fill(plaintext.begin(), plaintext.end(), 0);

    if (encrypted_content_b64.empty()) {
        std::cerr << "[ERROR] Ma hoa noi dung that bai.\n";
        return;
    }

    // 4. Encrypt File Key with Master Key (Key Wrapping).
    std::string wrapped_key = Crypto::wrapKey(file_key, master_key);

    // Xóa file_key khỏi RAM ngay lập tức
    std::fill(file_key.begin(), file_key.end(), 0);

    if (wrapped_key.empty()) {
        std::cerr << "[ERROR] Key Wrapping that bai.\n";
        return;
    }

    // 5. Send NoteData (encrypted_content, wrapped_key, iv_hex, filename) to Server via POST /upload.
    // Extract filename from filepath
    std::string filename = filepath;
    size_t last_slash = filepath.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        filename = filepath.substr(last_slash + 1);
    }
    
    NoteData payload;
    payload.note_id = 0; // Server sẽ tạo ID mới
    payload.encrypted_content = encrypted_content_b64;
    payload.wrapped_key = wrapped_key;
    payload.iv_hex = iv_b64;
    payload.filename = filename;
    payload.created_at = 0; // Server sẽ set timestamp 

    json j = payload;
    std::string response = net->post("/upload", j.dump());

    // 6. On success, display note_id to user.
    try {
        json j_resp = json::parse(response);
        if (j_resp.contains("success") && j_resp["success"].get<bool>()) {
            int note_id = j_resp.value("note_id", -1);
            std::cout << "[INFO] Upload file thanh cong. Note ID: " << note_id << "\n";
        } else {
            std::cerr << "[ERROR] Upload file that bai: " << j_resp.value("message", "Loi khong xac dinh") << "\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    }
}

void AppLogic::downloadFile(int note_id) {
    if (master_key.empty()) {
        std::cerr << "[ERROR] Chua dang nhap hoac Master Key khong ton tai. Hay dang nhap lai.\n";
        return;
    }

    // 1. Send GET /note/{id} to Server.
    std::string path = "/note/" + std::to_string(note_id);
    std::string response = net->get(path);

    try {
        json j_resp = json::parse(response);
        // Kiểm tra lỗi HTTP trước khi parse NoteData
        if (!j_resp.contains("encrypted_content")) {
             std::cerr << "[ERROR] Tai file that bai: " << j_resp.value("message", "Ghi chu khong ton tai hoac khong co quyen truy cap") << "\n";
             return;
        }

        // 2. Receive NoteData.
        NoteData payload = j_resp.get<NoteData>();
        
        // 3. Decrypt Wrapped Key using Master Key to get File Key.
        std::vector<unsigned char> file_key = Crypto::unwrapKey(payload.wrapped_key, master_key);
        
        if (file_key.empty() || file_key.size() != 32) {
             std::cerr << "[ERROR] Giai ma File Key that bai. Wrapped Key bi loi hoac Master Key khong dung.\n";
             return;
        }

        // 4. Decrypt Content using File Key.
        std::vector<unsigned char> iv = Crypto::base64Decode(payload.iv_hex);
        std::vector<unsigned char> encrypted_content = Crypto::base64Decode(payload.encrypted_content);
        
        if (iv.empty() || encrypted_content.empty()) {
             std::cerr << "[ERROR] Du lieu IV hoac Content bi loi (Base64).\n";
             // Xóa key
             std::fill(file_key.begin(), file_key.end(), 0);
             return;
        }

        std::vector<unsigned char> plaintext = Crypto::decryptAES(encrypted_content, file_key, iv);
        
        // Xóa file_key khỏi RAM ngay lập tức
        std::fill(file_key.begin(), file_key.end(), 0);

        if (plaintext.empty()) {
             std::cerr << "[ERROR] Giai ma noi dung that bai. Key hoac IV khong dung/Du lieu bi thay doi.\n";
             return;
        }

        // 5. Save decrypted content with original filename.
        std::string filename = "download/download_" + payload.filename;

        std::ofstream outfile(filename, std::ios::binary);
        if (outfile.is_open()) {
            outfile.write(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
            outfile.close();
            std::cout << "[INFO] Tai file thanh cong va luu vao: " << filename << "\n";
        } else {
            std::cerr << "[ERROR] Khong the luu file vao: " << filename << "\n";
        }

    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "[ERROR] Loi khong xac dinh khi tai file.\n";
    }
}

void AppLogic::listNotes() {
    // Requirement:
    // 1. Send GET /notes with auth token.
    std::string response = net->get("/notes");

    try {
        json j_resp = json::parse(response);
        // Server returns array of notes directly
        if (j_resp.is_array()) {
            std::cout << "\n--- DANH SACH GHI CHU CUA BAN ---\n";
            std::cout << std::setw(5) << "ID" << " | " << "NGAY TAO\n";
            std::cout << "--------------------------------------------------------\n";
            for (const auto& note : j_resp) {
                int note_id = note.value("note_id", -1);
                long created_at = note.value("created_at", 0L);
                
                // Convert timestamp to readable format (simple format)
                std::string time_str;
                if (created_at > 0) {
                    time_t timestamp = static_cast<time_t>(created_at);
                    char buffer[80];
                    struct tm* timeinfo = localtime(&timestamp);
                    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
                    time_str = buffer;
                } else {
                    time_str = "N/A";
                }
                
                std::cout << std::setw(5) << note_id << " | " << time_str << "\n";
            }
            std::cout << "--------------------------------------------------------\n";
        } else if (j_resp.contains("error")) {
            std::cerr << "[ERROR] Liet ke ghi chu that bai: " << j_resp.value("error", "Loi khong xac dinh") << "\n";
        } else {
            std::cerr << "[ERROR] Dinh dang phan hoi server khong hop le.\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
    }
}

void AppLogic::listMyShares() {
    if (master_key.empty()) {
        std::cerr << "[ERROR] Vui long dang nhap truoc!\n";
        return;
    }

    std::string response = net->get("/myshares");

    try {
        json j_resp = json::parse(response);
        
        if (j_resp.is_array()) {
            std::cout << "\n--- GHI CHU BAN DA CHIA SE VOI NGUOI KHAC ---\n";
            
            if (j_resp.empty()) {
                std::cout << "Ban chua chia se ghi chu nao.\n";
                return;
            }
            
            long currentTime = static_cast<long>(std::time(nullptr));
            
            for (const auto& share : j_resp) {
                int note_id = share.value("note_id", -1);
                std::string share_link = share.value("share_link", "");
                long expiration_time = share.value("expiration_time", 0L);
                bool is_expired = share.value("is_expired", false);
                auto shared_with = share.value("shared_with", json::array());
                
                std::cout << "\n[Note ID: " << note_id << "]\n";
                std::cout << "Link: " << share_link << "\n";
                
                // Convert timestamp to readable format
                time_t timestamp = static_cast<time_t>(expiration_time);
                char buffer[80];
                struct tm* timeinfo = localtime(&timestamp);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
                
                std::cout << "Het han: " << buffer;
                if (is_expired) {
                    std::cout << " (DA HET HAN)";
                }
                std::cout << "\n";
                
                std::cout << "Chia se voi: ";
                if (shared_with.is_array() && !shared_with.empty()) {
                    for (size_t i = 0; i < shared_with.size(); i++) {
                        std::cout << shared_with[i].get<std::string>();
                        if (i < shared_with.size() - 1) {
                            std::cout << ", ";
                        }
                    }
                } else {
                    std::cout << "(khong co nguoi dung nao)";
                }
                std::cout << "\n";
                std::cout << "--------------------------------------------------------\n";
            }
        } else if (j_resp.contains("error")) {
            std::cerr << "[ERROR] Liet ke chia se that bai: " << j_resp.value("error", "Loi khong xac dinh") << "\n";
        } else {
            std::cerr << "[ERROR] Dinh dang phan hoi server khong hop le.\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
    }
}

void AppLogic::deleteNote(int note_id) {
    if (master_key.empty()) {
        std::cerr << "[ERROR] Chua dang nhap. Hay dang nhap lai.\n";
        return;
    }

    // 1. Send DELETE /note/{id} with auth token
    std::string path = "/note/" + std::to_string(note_id);
    std::string response = net->del(path); 

    try {
        json j_resp = json::parse(response);
        if (j_resp.contains("success") && j_resp["success"].get<bool>()) {
            std::cout << "[INFO] Xoa ghi chu ID " << note_id << " thanh cong.\n";
        } else {
            std::cerr << "[ERROR] Xoa ghi chu ID " << note_id << " that bai: " << j_resp.value("message", "Khong co quyen hoac ID khong ton tai") << "\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    }
}

// Create Share Link
std::string AppLogic::createShareLink(int note_id, std::vector<std::string> allowed_usernames, int duration_seconds) {
    if (master_key.empty()) {
        std::cerr << "[ERROR] Chua dang nhap. Khong the tao link chia se.\n";
        return "";
    }

    // 1. Download note to get original wrapped_key and encrypted_content.
    std::string path_download = "/note/" + std::to_string(note_id);
    std::string response_download = net->get(path_download);
    
    NoteData payload;
    try {
        json j_resp = json::parse(response_download);
        if (!j_resp.contains("encrypted_content")) {
             std::cerr << "[ERROR] Khong the tai ghi chu ID " << note_id << " de chia se.\n";
             return "";
        }
        // Debug: print response to see what fields are present
        std::cout << "[DEBUG] Server response keys: ";
        for (auto& el : j_resp.items()) {
            std::cout << el.key() << " ";
        }
        std::cout << "\n";
        
        payload = j_resp.get<NoteData>();
    } catch (const std::exception& e) {
         std::cerr << "[ERROR] Loi phan tich NoteData khi tai ghi chu: " << e.what() << "\n";
         return "";
    } catch (...) {
         std::cerr << "[ERROR] Loi phan tich NoteData khi tai ghi chu (unknown).\n";
         return "";
    }

    // 2. Decrypt wrapped_key with master_key to get file_key.
    std::vector<unsigned char> file_key = Crypto::unwrapKey(payload.wrapped_key, master_key);
    if (file_key.empty()) {
        std::cerr << "[ERROR] Khong the giai ma File Key (Master Key loi?).\n";
        return "";
    }

    CreateShareLinkRequest link_req;
    link_req.note_id = note_id;
    link_req.duration_seconds = duration_seconds;
    
    // 3. For each username in allowed_usernames:
    for (const std::string& recipient_username : allowed_usernames) {
        // Lấy receive_public_key
        std::string path_pubkey = "/user/" + recipient_username + "/pubkey";
        std::string response_pubkey = net->get(path_pubkey);
        std::string recipient_receive_public_key;
        try {
            json j_resp = json::parse(response_pubkey);
            if (!j_resp.contains("receive_public_key_hex")) {
                 std::cerr << "[WARNING] Khong tim thay Public Key cua nguoi nhan " << recipient_username << ". Bo qua.\n";
                 continue;
            }
            recipient_receive_public_key = j_resp["receive_public_key_hex"].get<std::string>();
        } catch (...) {
             std::cerr << "[WARNING] Loi phan tich Public Key cua nguoi nhan. Bo qua.\n";
             continue;
        }

        // Tạo cặp khóa send_key tạm thời (ephemeral)
        DHKeyPair sendKeyPair = Crypto::generateECDHKeyPair();
        
        // Tính session_key = ECDH(send_private_key, receiver_receive_public_key)
        std::vector<unsigned char> session_key = Crypto::computeECDHSecret(sendKeyPair.privateKey, recipient_receive_public_key);
        
        if (session_key.empty()) {
            std::cerr << "[WARNING] Khong the tinh Session Key (ECDH that bai) cho " << recipient_username << ". Bo qua.\n";
            continue;
        }

        // Mã hóa file_key bằng session_key
        std::string new_wrapped_key = Crypto::wrapKey(file_key, session_key);

        // Xóa key tạm thời
        std::vector<unsigned char> send_private_key_bytes = Crypto::fromHex(sendKeyPair.privateKey);
        std::fill(send_private_key_bytes.begin(), send_private_key_bytes.end(), 0);
        std::fill(session_key.begin(), session_key.end(), 0);
        
        if (new_wrapped_key.empty()) {
            std::cerr << "[WARNING] Key Wrapping that bai cho " << recipient_username << ". Bo qua.\n";
            continue;
        }

        // Gửi danh sách {username, send_public_key, wrapped_key} lên Server
        ShareLinkUserAccess access_item;
        access_item.username = recipient_username;
        access_item.send_public_key_hex = sendKeyPair.publicKey;
        access_item.wrapped_key = new_wrapped_key;
        
        link_req.user_access_list.push_back(access_item);
    }
    
    // Xóa file key
    std::fill(file_key.begin(), file_key.end(), 0);

    // Gửi yêu cầu tạo link
    json j_link = link_req;
    std::string response_link = net->post("/share/link", j_link.dump());

    try {
        json j_resp = json::parse(response_link);
        if (j_resp.contains("share_link")) {
            std::string link = j_resp["share_link"].get<std::string>();
            std::cout << "[INFO] Tao link chia se thanh cong: " << link << "\n";
            return link;
        } else {
            std::cerr << "[ERROR] Tao link chia se that bai: " << j_resp.value("message", "Loi khong xac dinh") << "\n";
            return "";
        }
    } catch (...) {
        std::cerr << "[ERROR] Loi phan tich phan hoi server khi tao link chia se.\n";
        return "";
    }
}

void AppLogic::accessSharedNote(std::string share_token) {
    if (receive_private_key.empty()) {
        std::cerr << "[ERROR] Thieu Receive Private Key. Khong the truy cap link chia se!\n";
        return;
    }

    // Extract token from full URL if user pasted the complete link
    std::string token = share_token;
    size_t last_slash = share_token.find_last_of('/');
    if (last_slash != std::string::npos) {
        token = share_token.substr(last_slash + 1);
    }

    // 1. Send GET /share/{token} with auth token.
    std::string path = "/share/" + token;
    std::string response = net->get(path);

    try {
        json j_resp = json::parse(response);
        if (!j_resp.contains("encrypted_content")) {
             std::cerr << "[ERROR] Truy cap link that bai: " << j_resp.value("message", "Link khong hop le, da het han hoac khong co quyen truy cap") << "\n";
             return;
        }

        // 3. Nếu hợp lệ, trả về (encrypted_content, wrapped_key_for_this_user, iv)
        ShareLinkAccessResponse resp = j_resp.get<ShareLinkAccessResponse>();
        
        // Debug: check filename
        std::cout << "[DEBUG] Filename from server: '" << resp.filename << "'\n";
        
        // 4. Client dùng receive_private_key và sender_send_public_key để tính session_key
        std::vector<unsigned char> session_key = Crypto::computeECDHSecret(
            Crypto::toHex(receive_private_key), resp.send_public_key_hex
        );

        if (session_key.empty()) {
            std::cerr << "[ERROR] Khong the tinh Session Key (ECDH that bai).\n";
            return;
        }

        // 5. Giải mã wrapped_key bằng session_key -> file_key
        std::vector<unsigned char> file_key = Crypto::unwrapKey(resp.wrapped_key, session_key);

        // Xóa session_key ngay lập tức
        std::fill(session_key.begin(), session_key.end(), 0); 
        
        if (file_key.empty()) {
             std::cerr << "[ERROR] Giai ma File Key that bai. Session Key hoac Wrapped Key bi loi.\n";
             return;
        }

        // 6. Giải mã encrypted_content bằng file_key
        std::vector<unsigned char> iv = Crypto::base64Decode(resp.iv_hex);
        std::vector<unsigned char> encrypted_content = Crypto::base64Decode(resp.encrypted_content);

        std::vector<unsigned char> plaintext = Crypto::decryptAES(encrypted_content, file_key, iv);
        
        // Xóa file_key ngay lập tức
        std::fill(file_key.begin(), file_key.end(), 0); 
        
        if (plaintext.empty()) {
             std::cerr << "[ERROR] Giai ma noi dung that bai.\n";
             return;
        }

        // 7. Lưu nội dung đã giải mã với tên file gốc
        std::string filename = "download/shared_" + resp.filename;

        std::ofstream outfile(filename, std::ios::binary);
        if (outfile.is_open()) {
            outfile.write(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
            outfile.close();
            std::cout << "[INFO] Tai file chia se thanh cong va luu vao: " << filename << "\n";
        } else {
            std::cerr << "[ERROR] Khong the luu file vao: " << filename << "\n";
        }

    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    }
}

void AppLogic::revokeShare(std::string share_token) {
    // Extract token from full URL if user pasted the complete link
    std::string token = share_token;
    size_t last_slash = share_token.find_last_of('/');
    if (last_slash != std::string::npos) {
        token = share_token.substr(last_slash + 1);
    }

    // 1. Send DELETE /share/{token} with auth token
    std::string path = "/share/" + token;
    std::string response = net->del(path);

    try {
        json j_resp = json::parse(response);
        if (j_resp.contains("success") && j_resp["success"].get<bool>()) {
            std::cout << "[INFO] Huy chia se voi token " << token << " thanh cong.\n";
        } else {
            std::cerr << "[ERROR] Huy chia se that bai: " << j_resp.value("message", "Khong co quyen hoac token khong ton tai") << "\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    }
}





