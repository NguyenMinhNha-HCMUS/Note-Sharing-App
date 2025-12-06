#include "../vendor/crow_all.h"
#include "../vendor/json.hpp"
#include "Database.h"
#include "Auth.h"
#include "../common/Protocol.h"
#include "../common/Crypto.h"
#include <ctime>

using json = nlohmann::json;

int main() {
    Database db;
    if (!db.init()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return 1;
    }

    crow::SimpleApp app;

    // Root endpoint - API information
    CROW_ROUTE(app, "/")
    ([]() {
        json info;
        info["message"] = "Secure Note Sharing API Server";
        info["version"] = "1.0";
        info["endpoints"] = json::array({
            "POST /register - Register new user",
            "POST /login - User login",
            "POST /upload - Upload encrypted note (auth required)",
            "GET /notes - List user's notes (auth required)",
            "GET /note/<id> - Get note by ID (auth required)",
            "DELETE /note/<id> - Delete note (auth required)",
            "POST /share - Share note with user (auth required)",
            "POST /share/link - Create share link (auth required)",
            "GET /share/<token> - Access note via share link (auth required)",
            "DELETE /share/<token> - Revoke share link (auth required)",
            "GET /shared - List notes shared with user (auth required)",
            "GET /shared/<id> - Get shared note data (auth required)",
            "GET /user/<username>/pubkey - Get user's public key"
        });
        return crow::response(200, info.dump());
    });

    // API 1: Register new user
    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            
            std::string username = body["username"].get<std::string>();
            std::string password = body["password"].get<std::string>();
            std::string receivePubKey = body["receive_public_key_hex"].get<std::string>();
            
            if (username.empty() || password.empty()) {
                return crow::response(400, R"({"error": "Username and password required"})");
            }
            
            // Check if user already exists
            UserRecord existing = db.getUserByUsername(username);
            if (existing.id != -1) {
                return crow::response(400, R"({"error": "Username already exists"})");
            }
            
            // Generate 16-byte salt
            auto saltBytes = Crypto::generateRandomBytes(16);
            std::string salt = Crypto::toHex(saltBytes);
            
            // Hash password with salt: SHA256(password + salt)
            std::string passHash = Crypto::hashSHA256(password + salt);
            
            if (!db.createUser(username, passHash, salt, receivePubKey)) {
                return crow::response(500, R"({"error": "Failed to create user"})");
            }
            
            json response;
            response["success"] = true;
            response["message"] = "User registered successfully";
            return crow::response(200, response.dump());
            
        } catch (const std::exception& e) {
            return crow::response(400, R"({"error": "Invalid request body"})");
        }
    });

    // API 2: Login
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            
            std::string username = body["username"].get<std::string>();
            std::string password = body["password"].get<std::string>();
            
            UserRecord user = db.getUserByUsername(username);
            if (user.id == -1) {
                return crow::response(401, R"({"error": "Invalid credentials"})");
            }
            
            // Verify password
            std::string hashCheck = Crypto::hashSHA256(password + user.salt);
            if (hashCheck != user.password_hash) {
                return crow::response(401, R"({"error": "Invalid credentials"})");
            }
            
            // Generate session token
            std::string token = Auth::generateToken(user.id, username);
            
            json response;
            response["success"] = true;
            response["token"] = token;
            response["salt"] = user.salt;
            return crow::response(200, response.dump());
            
        } catch (const std::exception& e) {
            return crow::response(400, R"({"error": "Invalid request body"})");
        }
    });

    // API 3: Upload note (requires auth)
    CROW_ROUTE(app, "/upload").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        // Verify token
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        try {
            auto body = json::parse(req.body);
            
            std::string encryptedContent = body["encrypted_content"].get<std::string>();
            std::string wrappedKey = body["wrapped_key"].get<std::string>();
            std::string ivHex = body["iv_hex"].get<std::string>();
            
            int noteId = db.saveNote(auth.user_id, encryptedContent, wrappedKey, ivHex);
            if (noteId == -1) {
                return crow::response(500, R"({"error": "Failed to save note"})");
            }
            
            json response;
            response["success"] = true;
            response["note_id"] = noteId;
            return crow::response(200, response.dump());
            
        } catch (const std::exception& e) {
            return crow::response(400, R"({"error": "Invalid request body"})");
        }
    });

    // API 4: Get user's public key (for sharing)
    CROW_ROUTE(app, "/user/<string>/pubkey").methods(crow::HTTPMethod::GET)
    ([&db](std::string username) {
        UserRecord user = db.getUserByUsername(username);
        if (user.id == -1) {
            return crow::response(404, R"({"error": "User not found"})");
        }
        
        json response;
        response["username"] = user.username;
        response["receive_public_key_hex"] = user.receive_public_key_hex;
        return crow::response(200, response.dump());
    });

    // API 5: List user's notes
    CROW_ROUTE(app, "/notes").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        auto notes = db.getNotesForUser(auth.user_id);
        
        json response = json::array();
        for (const auto& note : notes) {
            json item;
            item["note_id"] = note.note_id;
            item["created_at"] = note.created_at;
            response.push_back(item);
        }
        
        return crow::response(200, response.dump());
    });

    // API 6: Get note by ID
    CROW_ROUTE(app, "/note/<int>").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req, int note_id) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        NoteData note = db.getNoteById(note_id);
        if (note.note_id == -1) {
            return crow::response(404, R"({"error": "Note not found"})");
        }
        
        // Verify ownership by checking if note exists in user's notes
        auto userNotes = db.getNotesForUser(auth.user_id);
        bool owned = false;
        for (const auto& n : userNotes) {
            if (n.note_id == note_id) {
                owned = true;
                break;
            }
        }
        
        if (!owned) {
            return crow::response(403, R"({"error": "Access denied"})");
        }
        
        json response;
        response["note_id"] = note.note_id;
        response["encrypted_content"] = note.encrypted_content;
        response["wrapped_key"] = note.wrapped_key;
        response["iv_hex"] = note.iv_hex;
        response["created_at"] = note.created_at;
        return crow::response(200, response.dump());
    });

    // API 7: Delete note
    CROW_ROUTE(app, "/note/<int>").methods(crow::HTTPMethod::DELETE)
    ([&db](const crow::request& req, int note_id) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        if (!db.deleteNote(note_id, auth.user_id)) {
            return crow::response(404, R"({"error": "Note not found or access denied"})");
        }
        
        json response;
        response["success"] = true;
        response["message"] = "Note deleted";
        return crow::response(200, response.dump());
    });

    // API 8: Share note with another user (user-to-user)
    CROW_ROUTE(app, "/share").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        try {
            auto body = json::parse(req.body);
            
            int noteId = body["note_id"].get<int>();
            std::string recipientUsername = body["recipient_username"].get<std::string>();
            std::string sendPubKey = body["send_public_key_hex"].get<std::string>();
            std::string newWrappedKey = body["new_wrapped_key"].get<std::string>();
            int duration = body["duration_seconds"].get<int>();
            
            // Get recipient user
            UserRecord recipient = db.getUserByUsername(recipientUsername);
            if (recipient.id == -1) {
                return crow::response(404, R"({"error": "Recipient not found"})");
            }
            
            if (!db.createUserShare(noteId, auth.user_id, recipient.id, 
                                    sendPubKey, newWrappedKey, duration)) {
                return crow::response(500, R"({"error": "Failed to create share"})");
            }
            
            json response;
            response["success"] = true;
            response["message"] = "Note shared with " + recipientUsername;
            return crow::response(200, response.dump());
            
        } catch (const std::exception& e) {
            return crow::response(400, R"({"error": "Invalid request body"})");
        }
    });

    // API 9: Create share link with username whitelist
    CROW_ROUTE(app, "/share/link").methods(crow::HTTPMethod::POST)
    ([&db](const crow::request& req) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        try {
            auto body = json::parse(req.body);
            
            int noteId = body["note_id"].get<int>();
            int duration = body["duration_seconds"].get<int>();
            auto userAccessList = body["user_access_list"];
            
            std::vector<Database::UserAccessEntry> accessList;
            for (const auto& item : userAccessList) {
                Database::UserAccessEntry entry;
                entry.username = item["username"].get<std::string>();
                entry.send_public_key_hex = item["send_public_key_hex"].get<std::string>();
                entry.wrapped_key = item["wrapped_key"].get<std::string>();
                accessList.push_back(entry);
            }
            
            std::string token = db.createShareLink(noteId, auth.user_id, accessList, duration);
            if (token.empty()) {
                return crow::response(500, R"({"error": "Failed to create share link"})");
            }
            
            long expirationAt = static_cast<long>(std::time(nullptr)) + duration;
            
            json response;
            response["success"] = true;
            response["share_link"] = "http://localhost:8080/share/" + token;
            response["token"] = token;
            response["expiration_at"] = expirationAt;
            return crow::response(200, response.dump());
            
        } catch (const std::exception& e) {
            return crow::response(400, R"({"error": "Invalid request body"})");
        }
    });

    // API 10: Access note via share link (requires login)
    CROW_ROUTE(app, "/share/<string>").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req, std::string shareToken) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Must be logged in to access shared notes"})");
        }
        
        auto data = db.getShareLinkData(shareToken, auth.username);
        if (!data.valid) {
            return crow::response(403, R"({"error": "Link expired or access denied"})");
        }
        
        json response;
        response["encrypted_content"] = data.encrypted_content;
        response["send_public_key_hex"] = data.send_public_key_hex;
        response["wrapped_key"] = data.wrapped_key;
        response["iv_hex"] = data.iv_hex;
        return crow::response(200, response.dump());
    });

    // API 11: Revoke share link
    CROW_ROUTE(app, "/share/<string>").methods(crow::HTTPMethod::DELETE)
    ([&db](const crow::request& req, std::string shareToken) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        if (!db.deleteShareLink(shareToken, auth.user_id)) {
            return crow::response(403, R"({"error": "Not owner or link not found"})");
        }
        
        json response;
        response["success"] = true;
        response["message"] = "Share link revoked";
        return crow::response(200, response.dump());
    });

    // API 12: List notes shared with current user
    CROW_ROUTE(app, "/shared").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        auto shareIds = db.getSharedNotesForUser(auth.user_id);
        
        json response = json::array();
        for (int id : shareIds) {
            json item;
            item["share_id"] = id;
            response.push_back(item);
        }
        
        return crow::response(200, response.dump());
    });

    // API 13: Get shared note data
    CROW_ROUTE(app, "/shared/<int>").methods(crow::HTTPMethod::GET)
    ([&db](const crow::request& req, int share_id) {
        std::string authHeader = req.get_header_value("Authorization");
        std::string tokenStr = Auth::extractToken(authHeader);
        TokenPayload auth = Auth::verifyToken(tokenStr);
        
        if (!auth.valid) {
            return crow::response(401, R"({"error": "Unauthorized"})");
        }
        
        auto info = db.getShareInfo(share_id, auth.user_id);
        if (info.note_id == -1) {
            return crow::response(404, R"({"error": "Share not found or expired"})");
        }
        
        json response;
        response["note_id"] = info.note_id;
        response["send_public_key_hex"] = info.send_public_key_hex;
        response["new_wrapped_key"] = info.new_wrapped_key;
        response["encrypted_content"] = info.encrypted_content;
        response["iv_hex"] = info.iv_hex;
        return crow::response(200, response.dump());
    });

    std::cout << "Server starting on port 8080..." << std::endl;
    app.port(8080).multithreaded().run();
    return 0;
}
