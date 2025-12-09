#include "network.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../vendor/httplib.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

Network::Network(std::string url) : base_url(url) {}

void Network::setToken(std::string token) {
    auth_token = token;
}

std::string Network::post(std::string path, std::string json_body) {
    // ------------------------------------------------------------------
    // --- BẮT ĐẦU PHẦN GIẢ LẬP (MOCK) DÀNH CHO KIỂM THỬ CLIENT ---
    // ------------------------------------------------------------------
    
    std::string mock_login_success = "{\"success\": true, \"token\": \"MOCK_JWT_TOKEN_123456\", \"salt\": \"30e5f22e842d075d9e56303038d7d3d0\", \"receivePrivateKey\": \"d7d3d090e5f22e842d075d9e56303038\", \"receivePublicKey\": \"e842d075d9e56303038d7d3d090e5f22\", \"message\": \"MOCK: Login/Register successful.\"}";
    
    if (path == "/register" || path == "/login") {
        std::cerr << "[MOCK] POST " << path << " - Bỏ qua mạng, trả về phản hồi giả lập." << std::endl;
        return mock_login_success;
    }
    
    // --- KẾT THÚC PHẦN GIẢ LẬP ---
    // ------------------------------------------------------------------

    httplib::Client cli(base_url);
    
    // Cài đặt header
    httplib::Headers headers;
    headers.emplace("Content-Type", "application/json");
    if (!auth_token.empty()) {
        headers.emplace("Authorization", "Bearer " + auth_token);
    }

    auto res = cli.Post(path.c_str(), headers, json_body, "application/json");

    if (res && res->status == 200) {
        return res->body;
    } else {
        std::stringstream ss;
        // Đảm bảo phản hồi lỗi cũng là JSON hợp lệ để AppLogic có thể parse
        ss << "{\"success\": false, \"status\": " << (res ? res->status : 0) << ", \"message\": \"HTTP Error or connection failed (Status: " << (res ? res->status : 0) << ").\"}";
        std::cerr << "[NET] POST " << path << " failed with status " << (res ? res->status : 0) << std::endl;
        return ss.str();
    }
}

std::string Network::get(std::string path) {
    // ------------------------------------------------------------------
    // --- BẮT ĐẦU PHẦN GIẢ LẬP (MOCK) DÀNH CHO GET ---
    // ------------------------------------------------------------------

    // Giả lập cho /note/list
    if (path == "/note/list") {
        std::string mock_response_list = "{\"notes\": [{\"noteId\": 101, \"createdAt\": \"2023-10-27T10:00:00Z\", \"metadata\": \"Title 1\"}, {\"noteId\": 102, \"createdAt\": \"2023-10-27T11:00:00Z\", \"metadata\": \"Title 2\"}]}";
        std::cerr << "[MOCK] GET /note/list - Tra ve danh sach gia lap." << std::endl;
        return mock_response_list;
    }
    
    // Giả lập cho /note/{id} (để test Download)
    if (path.find("/note/") == 0 && path.length() > 6) {
        std::string mock_encrypted_content = "MOCK_ENCRYPTED_CONTENT_BASE64_ABCDEF123456";
        std::string mock_wrapped_key = "MOCK_WRAPPED_KEY_HEX_1234567890";
        std::string mock_iv = "MOCK_IV_BASE64_9876543210";
        
        std::stringstream ss;
        ss << "{\"encryptedContent\": \"" << mock_encrypted_content << "\", ";
        ss << "\"wrappedKey\": \"" << mock_wrapped_key << "\", ";
        ss << "\"iv\": \"" << mock_iv << "\", ";
        ss << "\"metadata\": \"Mocked Note Title\"}";
        
        std::cerr << "[MOCK] GET " << path << " - Tra ve NotePayload gia lap." << std::endl;
        return ss.str();
    }

    // --- KẾT THÚC PHẦN GIẢ LẬP ---
    // ------------------------------------------------------------------
    
    httplib::Client cli(base_url);
    
    // Cài đặt header
    httplib::Headers headers;
    if (!auth_token.empty()) {
        headers.emplace("Authorization", "Bearer " + auth_token);
    }

    auto res = cli.Get(path.c_str(), headers);

    if (res && res->status == 200) {
        return res->body;
    } else {
        std::stringstream ss;
        ss << "{\"success\": false, \"status\": " << (res ? res->status : 0) << ", \"message\": \"HTTP Error or connection failed (Status: " << (res ? res->status : 0) << ").\"}";
        std::cerr << "[NET] GET " << path << " failed with status " << (res ? res->status : 0) << std::endl;
        return ss.str();
    }
}