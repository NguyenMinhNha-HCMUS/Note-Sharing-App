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

std::string Network::del(std::string path) {
    httplib::Client cli(base_url);
    
    // Cài đặt header
    httplib::Headers headers;
    if (!auth_token.empty()) {
        headers.emplace("Authorization", "Bearer " + auth_token);
    }

    auto res = cli.Delete(path.c_str(), headers);

    if (res && res->status == 200) {
        return res->body;
    } else {
        std::stringstream ss;
        ss << "{\"success\": false, \"status\": " << (res ? res->status : 0) << ", \"message\": \"HTTP Error or connection failed (Status: " << (res ? res->status : 0) << ").\"}";
        std::cerr << "[NET] DELETE " << path << " failed with status " << (res ? res->status : 0) << std::endl;
        return ss.str();
    }
}
