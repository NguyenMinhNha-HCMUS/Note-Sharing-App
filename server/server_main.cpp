#include "../vendor/crow_all.h"
#include "Database.h"
#include "Auth.h"
#include "../common/Protocol.h"
#include "../common/Crypto.h"

int main() {
    Database db;
    db.init(); // Tạo bảng

    crow::SimpleApp app;

    // API 1: Đăng ký
    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        // Requirement: 
        // 1. Parse JSON body to get AuthRequest (username, password, receive_public_key_hex).
        // 2. Generate a random salt (16 bytes).
        // 3. Hash the password with the salt using SHA-256.
        // 4. Call db.createUser to store username, password hash, salt, and public key.
        // 5. Return 200 OK if successful, 400 Bad Request otherwise.
        return crow::response(200, "Skeleton: Register success");
    });

    // API 2: Đăng nhập
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        // Requirement:
        // 1. Parse JSON body to get AuthRequest (username, password).
        // 2. Retrieve user from DB by username.
        // 3. Re-hash the received password with the stored salt.
        // 4. Compare the computed hash with the stored hash.
        // 5. If match: Generate a session token (JWT), return 200 with LoginResponse (token, salt).
        // 6. If mismatch: Return 401 Unauthorized.
        return crow::response(200, "Skeleton: Login success");
    });

    // API 3: Upload Ghi chú (Cần Token)
    CROW_ROUTE(app, "/upload").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        // Requirement:
        // 1. Verify Authorization header (Token).
        // 2. Parse JSON body to get NotePayload (encrypted_content, wrapped_key, iv).
        // 3. Save the note to DB associated with the user_id from token.
        // 4. Return 200 OK if saved, 500 Internal Error otherwise.
        return crow::response(200, "Skeleton: Upload success");
    });

    // API 4: Lấy Public Key của người dùng khác (để chia sẻ file)
    CROW_ROUTE(app, "/user/<string>/pubkey").methods(crow::HTTPMethod::GET)
    ([&db](std::string username) {
        // Requirement:
        // 1. Look up user by username in DB.
        // 2. If found, return PublicKeyPayload (username, receive_public_key_hex).
        // 3. If not found, return 404.
        return crow::response(200, "Skeleton: Public Key Retrieved");
    });

    // API 5: Liệt kê ghi chú của user
    CROW_ROUTE(app, "/notes").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req) {
        // Requirement:
        // 1. Verify Authorization token.
        // 2. Extract user_id from token.
        // 3. Call db.getNotesForUser(user_id).
        // 4. Return JSON array of notes (id, created_at).
        return crow::response(200, "Skeleton: Notes listed");
    });

    // API 6: Tải ghi chú theo ID
    CROW_ROUTE(app, "/note/<int>").methods(crow::HTTPMethod::GET)
    ([&db](int note_id) {
        // Requirement:
        // 1. Verify Authorization token.
        // 2. Check user owns the note.
        // 3. Return NoteData (encrypted_content, wrapped_key, iv).
        return crow::response(200, "Skeleton: Note downloaded");
    });

    // API 7: Xóa ghi chú
    CROW_ROUTE(app, "/note/<int>").methods(crow::HTTPMethod::DELETE)
    ([&db](const crow::request& req, int note_id) {
        // Requirement:
        // 1. Verify Authorization token.
        // 2. Extract user_id from token.
        // 3. Call db.deleteNote(note_id, user_id).
        // 4. Return 200 OK if deleted, 403 if not owner, 404 if not found.
        return crow::response(200, "Skeleton: Note deleted");
    });

    // API 8: Chia sẻ ghi chú với user khác (User-to-User)
    CROW_ROUTE(app, "/share").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        // Requirement:
        // 1. Verify Authorization token, get sender_id.
        // 2. Parse NoteShareRequest (note_id, recipient_username, send_public_key_hex, new_wrapped_key, duration_time).
        // 3. Look up recipient by username to get recipient_id.
        // 4. Call db.createUserShare(note_id, sender_id, recipient_id, ...).
        // 5. Return 200 OK with share confirmation.
        return crow::response(200, "Skeleton: Note shared with user");
    });

    // API 9: Tạo link chia sẻ với whitelist username
    CROW_ROUTE(app, "/share/link").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        // Requirement:
        // 1. Verify Authorization token, get user_id.
        // 2. Parse request body: {note_id, user_access_list: [{username, send_public_key_hex, wrapped_key}], duration_seconds}.
        // 3. Verify user owns the note.
        // 4. Call db.createShareLink(note_id, user_id, user_access_list, duration_seconds).
        // 5. Return {share_link: "http://localhost:8080/share/{token}", expiration_at}.
        return crow::response(200, "Skeleton: Share link created");
    });

    // API 10: Truy cập ghi chú qua link (CẦN ĐĂNG NHẬP)
    CROW_ROUTE(app, "/share/<string>").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req, std::string token) {
        // Requirement:
        // 1. Verify Authorization token, get current_username.
        // 2. Call db.getShareLinkData(token, current_username).
        // 3. If invalid/expired/not authorized, return 403 Forbidden or 404.
        // 4. If valid, return {encrypted_content, send_public_key_hex, wrapped_key, iv}.
        // 5. Client sẽ dùng ECDH để giải mã.
        return crow::response(200, "Skeleton: Shared note accessed");
    });

    // API 11: Hủy link chia sẻ
    CROW_ROUTE(app, "/share/<string>").methods(crow::HTTPMethod::DELETE)
    ([&db](const crow::request& req, std::string token) {
        // Requirement:
        // 1. Verify Authorization token, get user_id.
        // 2. Call db.deleteShareLink(token, user_id).
        // 3. Return 200 OK if deleted, 403 if not owner.
        return crow::response(200, "Skeleton: Share link revoked");
    });

    // API 12: Lấy ghi chú được chia sẻ cho user hiện tại
    CROW_ROUTE(app, "/shared").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req) {
        // Requirement:
        // 1. Verify Authorization token, get user_id.
        // 2. Call db.getSharedNotesForUser(user_id).
        // 3. Return list of share IDs.
        return crow::response(200, "Skeleton: Shared notes listed");
    });

    // API 13: Nhận ghi chú được chia sẻ
    CROW_ROUTE(app, "/shared/<int>").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req, int share_id) {
        // Requirement:
        // 1. Verify Authorization token, get user_id.
        // 2. Call db.getShareInfo(share_id, user_id).
        // 3. Return ShareInfo (note_id, send_public_key_hex, new_wrapped_key, encrypted_content, iv).
        return crow::response(200, "Skeleton: Shared note data retrieved");
    });

    // Chạy Server ở cổng 8080
    app.port(8080).multithreaded().run();
}
