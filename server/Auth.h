#pragma once
#include <string>

struct TokenPayload {
    int user_id;
    std::string username;
    bool valid;
};

class Auth {
public:
    // Generate a signed token from user_id and username
    static std::string generateToken(int user_id, std::string username);
    
    // Verify token and extract payload. Returns valid=false if invalid.
    static TokenPayload verifyToken(std::string token);
    
    // Helper to extract token from Authorization header (handles "Bearer " prefix)
    static std::string extractToken(const std::string& authHeader);
};
