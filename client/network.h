// Client/Network.h
#pragma once
#include <string>

class Network {
private:
    std::string base_url; // "http://localhost:8080"
    std::string auth_token;

public:
    Network(std::string url);
    void setToken(std::string token);
    
    // Gửi POST request với body JSON
    std::string post(std::string path, std::string json_body);
    
    // Gửi GET request
    std::string get(std::string path);
};
