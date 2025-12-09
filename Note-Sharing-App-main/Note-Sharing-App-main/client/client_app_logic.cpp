// Thêm vào client_app_logic.cpp (và sửa các hàm hiện tại)
#include "client_app_logic.h"
#include "../common/Crypto.h"
#include "../common/Protocol.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm> // for std::min, std::max
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
    req.receivePublicKey = keyPair.publicKey;

    json j = req;
    std::string json_body = j.dump();

    // 3. Send POST /register to Server.
    std::string response = net->post("/register", json_body);

    // 4. Return true if Server responds 200 OK.
    try {
        json j_resp = json::parse(response);
        if (j_resp.contains("success") && j_resp["success"].get<bool>()) {
            std::cout << "[INFO] Dang ky thanh cong!\n";
            // LƯU Ý: Khóa riêng tư này phải được lưu an toàn cho người dùng! 
            // Nếu bạn muốn lưu vào đĩa (sau khi mã hóa bằng Master Key), hãy làm ở đây.
            // Trong khuôn khổ lab, ta chỉ tạm lưu vào RAM
            receive_private_key = Crypto::fromHex(keyPair.privateKey);
            receive_public_key = Crypto::fromHex(keyPair.publicKey);
            current_username = user;
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
    req.receivePublicKey = ""; 

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

            // 5. Store Master Key in RAM. (Done above)
            // LƯU Ý: Khóa riêng tư nhận file đã được mã hóa/lưu an toàn trên server. 
            // Cần giải mã và lưu vào RAM (receive_private_key).
            if (!resp.receivePrivateKey.empty()) {
                // Giả định resp.receivePrivateKey là khóa riêng đã được mã hóa bằng Master Key
                receive_private_key = Crypto::unwrapKey(resp.receivePrivateKey, master_key);
                receive_public_key = Crypto::fromHex(resp.receivePublicKey);
                if (receive_private_key.empty()) {
                    std::cerr << "[WARNING] Khong the giai ma Receive Private Key. Khong the nhan file chia se!\n";
                }
            } else {
                 std::cerr << "[WARNING] Khong co Receive Private Key tu server. Khong the nhan file chia se!\n";
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
    std::vector<unsigned char> encrypted_content_bytes = Crypto::AESEncrypt(plaintext, file_key, iv);
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

    // 5. Send NotePayload (encrypted_content, wrapped_key, iv) to Server via POST /upload.
    NotePayload payload;
    payload.encryptedContent = encrypted_content_b64;
    payload.wrappedKey = wrapped_key;
    payload.iv = iv_b64;
    // Lấy tên file làm metadata
    payload.metadata = filepath.substr(filepath.find_last_of("/\\") + 1); 

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
        // Kiểm tra lỗi HTTP trước khi parse NotePayload
        if (!j_resp.contains("encryptedContent")) {
             std::cerr << "[ERROR] Tai file that bai: " << j_resp.value("message", "Ghi chu khong ton tai hoac khong co quyen truy cap") << "\n";
             return;
        }

        // 2. Receive NotePayload.
        NotePayload payload = j_resp.get<NotePayload>();
        
        // 3. Decrypt Wrapped Key using Master Key to get File Key.
        std::vector<unsigned char> file_key = Crypto::unwrapKey(payload.wrappedKey, master_key);
        
        if (file_key.empty() || file_key.size() != 32) {
             std::cerr << "[ERROR] Giai ma File Key that bai. Wrapped Key bi loi hoac Master Key khong dung.\n";
             return;
        }

        // 4. Decrypt Content using File Key.
        std::vector<unsigned char> iv = Crypto::base64Decode(payload.iv);
        std::vector<unsigned char> encrypted_content = Crypto::base64Decode(payload.encryptedContent);
        
        if (iv.empty() || encrypted_content.empty()) {
             std::cerr << "[ERROR] Du lieu IV hoac Content bi loi (Base64).\n";
             // Xóa key
             std::fill(file_key.begin(), file_key.end(), 0);
             return;
        }

        std::vector<unsigned char> plaintext = Crypto::AESDecrypt(encrypted_content, file_key, iv);
        
        // Xóa file_key khỏi RAM ngay lập tức
        std::fill(file_key.begin(), file_key.end(), 0);

        if (plaintext.empty()) {
             std::cerr << "[ERROR] Giai ma noi dung that bai. Key hoac IV khong dung/Du lieu bi thay doi.\n";
             return;
        }

        // 5. Save decrypted content to "download/note_{note_id}.txt" or original filename if stored.
        std::string filename = payload.metadata.empty() 
                               ? "download/note_" + std::to_string(note_id) + ".txt" 
                               : "download/" + payload.metadata;

        // Đảm bảo tên file không chứa ký tự cấm và an toàn
        // (Bỏ qua bước này để đơn giản hóa, nhưng cần thiết trong thực tế)

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
        // Server returns list of notes (id, created_at, metadata) for current user.
        if (j_resp.contains("notes")) {
            std::cout << "\n--- DANH SACH GHI CHU CUA BAN ---\n";
            std::cout << std::setw(5) << "ID" << " | " << std::setw(30) << "TEN FILE/METADATA" << " | " << "NGAY TAO\n";
            std::cout << "--------------------------------------------------------\n";
            for (const auto& note : j_resp["notes"]) {
                std::cout << std::setw(5) << note.value("note_id", -1) 
                          << " | " << std::setw(30) << std::left << note.value("metadata", "N/A") << std::right 
                          << " | " << note.value("created_at", "N/A") << "\n";
            }
            std::cout << "--------------------------------------------------------\n";
        } else {
            std::cerr << "[ERROR] Liet ke ghi chu that bai: " << j_resp.value("message", "Loi khong xac dinh hoac chua dang nhap") << "\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    }
}

void AppLogic::deleteNote(int note_id) {
    if (master_key.empty()) {
        std::cerr << "[ERROR] Chua dang nhap. Hay dang nhap lai.\n";
        return;
    }

    // 1. Send DELETE /note/{id} with auth token (POST for simplicity with basic httplib)
    std::string path = "/note/delete/" + std::to_string(note_id); 
    // Giả định server hỗ trợ DELETE hoặc dùng POST với body rỗng cho thao tác này
    std::string response = net->post(path, "{}"); 

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

// --------------------------------------------------------
// MARK: - Sharing (E2EE - User to User)
// --------------------------------------------------------

void AppLogic::shareNoteWithUser(int note_id, std::string recipient_username, int duration_seconds) {
    if (master_key.empty() || receive_private_key.empty()) {
        std::cerr << "[ERROR] Chua dang nhap hoac thieu Receive Key. Khong the chia se E2EE.\n";
        return;
    }

    // 1. Download note to get NotePayload.
    std::string path_download = "/note/" + std::to_string(note_id);
    std::string response_download = net->get(path_download);
    
    NotePayload payload;
    try {
        json j_resp = json::parse(response_download);
        if (!j_resp.contains("encryptedContent")) {
             std::cerr << "[ERROR] Khong the tai ghi chu ID " << note_id << " de chia se.\n";
             return;
        }
        payload = j_resp.get<NotePayload>();
    } catch (...) {
         std::cerr << "[ERROR] Loi phan tich NotePayload khi tai ghi chu.\n";
         return;
    }

    // 2. Decrypt wrapped_key with master_key to get file_key.
    std::vector<unsigned char> file_key = Crypto::unwrapKey(payload.wrappedKey, master_key);
    if (file_key.empty()) {
        std::cerr << "[ERROR] Khong the giai ma File Key (Master Key loi?).\n";
        return;
    }
    
    // 3. Get recipient's receive public key.
    std::string path_pubkey = "/user/pubkey/" + recipient_username;
    std::string response_pubkey = net->get(path_pubkey);
    std::string recipient_receive_public_key;
    try {
        json j_resp = json::parse(response_pubkey);
        if (!j_resp.contains("publicKey")) {
             std::cerr << "[ERROR] Khong tim thay Public Key cua nguoi nhan " << recipient_username << ".\n";
             // Xóa key
             std::fill(file_key.begin(), file_key.end(), 0);
             return;
        }
        recipient_receive_public_key = j_resp["publicKey"].get<std::string>();
    } catch (...) {
         std::cerr << "[ERROR] Loi phan tich Public Key cua nguoi nhan.\n";
         // Xóa key
         std::fill(file_key.begin(), file_key.end(), 0);
         return;
    }

    // 4. Generate ephemeral ECDH send_key pair.
    DHKeyPair sendKeyPair = Crypto::generateECDHKeyPair();
    
    // 5. Compute session_key = ECDH(send_private_key, receiver_receive_public_key).
    std::vector<unsigned char> session_key = Crypto::computeECDHSecret(sendKeyPair.privateKey, recipient_receive_public_key);
    
    if (session_key.empty()) {
        std::cerr << "[ERROR] Khong the tinh Session Key (ECDH that bai).\n";
        // Xóa key
        std::fill(file_key.begin(), file_key.end(), 0);
        return;
    }

    // 6. Wrap file_key with session_key to create new_wrapped_key.
    std::string new_wrapped_key = Crypto::wrapKey(file_key, session_key);

    // 7. Send NoteShareRequest (note_id, recipient_username, send_public_key, new_wrapped_key, duration_seconds) to Server.
    NoteShareRequest share_req;
    share_req.noteId = note_id;
    share_req.recipientUsername = recipient_username;
    share_req.sendPublicKey = sendKeyPair.publicKey;
    share_req.newWrappedKey = new_wrapped_key;
    share_req.durationSeconds = duration_seconds;

    json j_share = share_req;
    std::string response_share = net->post("/share/user", j_share.dump());

    // 8. IMMEDIATELY destroy send_private_key and session_key from memory.
    std::vector<unsigned char> send_private_key_bytes = Crypto::fromHex(sendKeyPair.privateKey);
    std::fill(send_private_key_bytes.begin(), send_private_key_bytes.end(), 0);
    std::fill(session_key.begin(), session_key.end(), 0);
    std::fill(file_key.begin(), file_key.end(), 0); // Xóa File Key

    try {
        json j_resp = json::parse(response_share);
        if (j_resp.contains("success") && j_resp["success"].get<bool>()) {
            std::cout << "[INFO] Chia se ghi chu ID " << note_id << " voi " << recipient_username << " thanh cong.\n";
        } else {
            std::cerr << "[ERROR] Chia se that bai: " << j_resp.value("message", "Loi khong xac dinh") << "\n";
        }
    } catch (...) {
        std::cerr << "[ERROR] Loi phan tich phan hoi server khi chia se.\n";
    }
}

void AppLogic::receiveSharedNote(int share_id) {
    if (receive_private_key.empty()) {
        std::cerr << "[ERROR] Thieu Receive Private Key. Khong the nhan file chia se!\n";
        return;
    }

    // 1. Send GET /shared/{share_id} with auth token.
    std::string path = "/shared/" + std::to_string(share_id);
    std::string response = net->get(path);

    try {
        json j_resp = json::parse(response);
        if (!j_resp.contains("sendPublicKey")) {
             std::cerr << "[ERROR] Tai thong tin chia se that bai: " << j_resp.value("message", "ID chia se khong ton tai hoac da het han") << "\n";
             return;
        }

        // 4. If valid, server returns {note_id, send_public_key, new_wrapped_key, encrypted_content, iv}.
        SharedNoteReceiveResponse resp = j_resp.get<SharedNoteReceiveResponse>();
        
        // 5. Compute session_key = ECDH(receive_private_key, sender_send_public_key).
        std::vector<unsigned char> session_key = Crypto::computeECDHSecret(
            Crypto::toHex(receive_private_key), resp.sendPublicKey
        );

        if (session_key.empty()) {
            std::cerr << "[ERROR] Khong the tinh Session Key (ECDH that bai).\n";
            return;
        }

        // 6. Decrypt new_wrapped_key with session_key to get file_key.
        std::vector<unsigned char> file_key = Crypto::unwrapKey(resp.newWrappedKey, session_key);

        // Xóa session_key ngay lập tức
        std::fill(session_key.begin(), session_key.end(), 0); 
        
        if (file_key.empty()) {
             std::cerr << "[ERROR] Giai ma File Key that bai. Session Key hoac Wrapped Key bi loi.\n";
             return;
        }

        // 7. Decrypt encrypted_content with file_key.
        std::vector<unsigned char> iv = Crypto::base64Decode(resp.iv);
        std::vector<unsigned char> encrypted_content = Crypto::base64Decode(resp.encryptedContent);

        std::vector<unsigned char> plaintext = Crypto::AESDecrypt(encrypted_content, file_key, iv);
        
        // Xóa file_key ngay lập tức
        std::fill(file_key.begin(), file_key.end(), 0); 
        
        if (plaintext.empty()) {
             std::cerr << "[ERROR] Giai ma noi dung that bai.\n";
             return;
        }

        // 8. Save decrypted content.
        std::string filename = "download/shared_by_" + std::to_string(resp.noteId) + "_at_" + std::to_string(share_id) + ".txt";

        std::ofstream outfile(filename, std::ios::binary);
        if (outfile.is_open()) {
            outfile.write(reinterpret_cast<const char*>(plaintext.data()), plaintext.size());
            outfile.close();
            std::cout << "[INFO] Nhận và luu file chia se thanh cong vao: " << filename << "\n";
            // Xóa file chia sẻ trên server nếu cần (tùy vào yêu cầu lab)
            // net->post("/shared/delete/" + std::to_string(share_id), "{}");
        } else {
            std::cerr << "[ERROR] Khong the luu file vao: " << filename << "\n";
        }

    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    }
}

// --------------------------------------------------------
// MARK: - Sharing (Time-Sensitive Link)
// --------------------------------------------------------

std::string AppLogic::createShareLink(int note_id, std::vector<std::string> allowed_usernames, int duration_seconds) {
    if (master_key.empty()) {
        std::cerr << "[ERROR] Chua dang nhap. Khong the tao link chia se.\n";
        return "";
    }

    // 1. Download note to get original wrapped_key and encrypted_content.
    std::string path_download = "/note/" + std::to_string(note_id);
    std::string response_download = net->get(path_download);
    
    NotePayload payload;
    try {
        json j_resp = json::parse(response_download);
        if (!j_resp.contains("encryptedContent")) {
             std::cerr << "[ERROR] Khong the tai ghi chu ID " << note_id << " de chia se.\n";
             return "";
        }
        payload = j_resp.get<NotePayload>();
    } catch (...) {
         std::cerr << "[ERROR] Loi phan tich NotePayload khi tai ghi chu.\n";
         return "";
    }

    // 2. Decrypt wrapped_key with master_key to get file_key.
    std::vector<unsigned char> file_key = Crypto::unwrapKey(payload.wrappedKey, master_key);
    if (file_key.empty()) {
        std::cerr << "[ERROR] Khong the giai ma File Key (Master Key loi?).\n";
        return "";
    }

    ShareLinkRequest link_req;
    link_req.noteId = note_id;
    link_req.durationSeconds = duration_seconds;
    
    // 3. For each username in allowed_usernames:
    for (const std::string& recipient_username : allowed_usernames) {
        // Lấy receive_public_key
        std::string path_pubkey = "/user/pubkey/" + recipient_username;
        std::string response_pubkey = net->get(path_pubkey);
        std::string recipient_receive_public_key;
        try {
            json j_resp = json::parse(response_pubkey);
            if (!j_resp.contains("publicKey")) {
                 std::cerr << "[WARNING] Khong tim thay Public Key cua nguoi nhan " << recipient_username << ". Bo qua.\n";
                 continue;
            }
            recipient_receive_public_key = j_resp["publicKey"].get<std::string>();
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
        UserAccessItem access_item;
        access_item.username = recipient_username;
        access_item.sendPublicKey = sendKeyPair.publicKey;
        access_item.wrappedKey = new_wrapped_key;
        
        link_req.userAccessList.push_back(access_item);
    }
    
    // Xóa file key
    std::fill(file_key.begin(), file_key.end(), 0);

    // Gửi yêu cầu tạo link
    json j_link = link_req;
    std::string response_link = net->post("/share/link", j_link.dump());

    try {
        json j_resp = json::parse(response_link);
        if (j_resp.contains("token")) {
            std::string token = j_resp["token"].get<std::string>();
            std::cout << "[INFO] Tao link chia se thanh cong. Token: " << token << "\n";
            return "http://localhost:8080/share/link/" + token;
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

    // 1. Send GET /share/{token} with auth token.
    std::string path = "/share/link/" + share_token;
    std::string response = net->get(path);

    try {
        json j_resp = json::parse(response);
        if (!j_resp.contains("encryptedContent")) {
             std::cerr << "[ERROR] Truy cap link that bai: " << j_resp.value("message", "Link khong hop le, da het han hoac khong co quyen truy cap") << "\n";
             return;
        }

        // 3. Nếu hợp lệ, trả về (encrypted_content, wrapped_key_for_this_user, iv)
        SharedNoteAccessResponse resp = j_resp.get<SharedNoteAccessResponse>();
        
        // 4. Client dùng receive_private_key và sender_send_public_key để tính session_key
        std::vector<unsigned char> session_key = Crypto::computeECDHSecret(
            Crypto::toHex(receive_private_key), resp.sendPublicKey
        );

        if (session_key.empty()) {
            std::cerr << "[ERROR] Khong the tinh Session Key (ECDH that bai).\n";
            return;
        }

        // 5. Giải mã wrapped_key bằng session_key -> file_key
        std::vector<unsigned char> file_key = Crypto::unwrapKey(resp.wrappedKey, session_key);

        // Xóa session_key ngay lập tức
        std::fill(session_key.begin(), session_key.end(), 0); 
        
        if (file_key.empty()) {
             std::cerr << "[ERROR] Giai ma File Key that bai. Session Key hoac Wrapped Key bi loi.\n";
             return;
        }

        // 6. Giải mã encrypted_content bằng file_key
        std::vector<unsigned char> iv = Crypto::base64Decode(resp.iv);
        std::vector<unsigned char> encrypted_content = Crypto::base64Decode(resp.encryptedContent);

        std::vector<unsigned char> plaintext = Crypto::AESDecrypt(encrypted_content, file_key, iv);
        
        // Xóa file_key ngay lập tức
        std::fill(file_key.begin(), file_key.end(), 0); 
        
        if (plaintext.empty()) {
             std::cerr << "[ERROR] Giai ma noi dung that bai.\n";
             return;
        }

        // 7. Lưu nội dung đã giải mã
        std::string filename = "download/shared_link_" + share_token.substr(0, 8) + ".txt";

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
    // Requirement:
    // 1. Send DELETE /share/{token} with auth token. (Using POST for simplicity)
    std::string path = "/share/revoke/" + share_token;
    
    std::string response = net->post(path, "{}");

    try {
        json j_resp = json::parse(response);
        if (j_resp.contains("success") && j_resp["success"].get<bool>()) {
            std::cout << "[INFO] Huy chia se voi token " << share_token << " thanh cong.\n";
        } else {
            std::cerr << "[ERROR] Huy chia se that bai: " << j_resp.value("message", "Khong co quyen hoac token khong ton tai") << "\n";
        }
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] Phan tich phan hoi server that bai: " << e.what() << "\n";
    }
}