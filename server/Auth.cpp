#include "Auth.h"
#include "../common/Crypto.h"
#include <sstream>
#include <ctime>

// Server secret key for signing tokens (in production, load from env or config)
static const std::string SERVER_SECRET = "SecureNoteApp_ServerSecret_2024";

// Token time-to-live: 30 minutes
static const long long TOKEN_TTL_SECONDS = 1800;

std::string Auth::generateToken(int user_id, std::string username) {
    // Token format: base64(user_id:username:exp).signature
    // where signature = SHA256(secret + base_part)
    
    const long long exp = static_cast<long long>(std::time(nullptr)) + TOKEN_TTL_SECONDS;
    std::string basePart = std::to_string(user_id) + ":" + username + ":" + std::to_string(exp);
    
    // Encode base part to avoid issues with special characters
    std::vector<unsigned char> baseBytes(basePart.begin(), basePart.end());
    std::string encodedBase = Crypto::base64Encode(baseBytes);
    
    // Create signature
    std::string toSign = SERVER_SECRET + encodedBase;
    std::string signature = Crypto::hashSHA256(toSign);
    
    return encodedBase + "." + signature;
}

TokenPayload Auth::verifyToken(std::string token) {
    TokenPayload payload{-1, "", false};
    payload.exp = -1;
    
    // Find the separator
    size_t dotPos = token.find('.');
    if (dotPos == std::string::npos) {
        return payload;
    }
    
    std::string encodedBase = token.substr(0, dotPos);
    std::string providedSig = token.substr(dotPos + 1);
    
    // Verify signature
    std::string toSign = SERVER_SECRET + encodedBase;
    std::string expectedSig = Crypto::hashSHA256(toSign);
    
    if (providedSig != expectedSig) {
        return payload;
    }
    
    // Decode base part
    std::vector<unsigned char> decodedBytes = Crypto::base64Decode(encodedBase);
    if (decodedBytes.empty()) {
        return payload;
    }
    
    std::string basePart(decodedBytes.begin(), decodedBytes.end());
    
    // Parse user_id:username:exp
    size_t firstColon = basePart.find(':');
    size_t secondColon = basePart.find(':', firstColon == std::string::npos ? std::string::npos : firstColon + 1);
    if (firstColon == std::string::npos || secondColon == std::string::npos) {
        return payload;
    }
    
    std::string userIdStr = basePart.substr(0, firstColon);
    std::string username = basePart.substr(firstColon + 1, secondColon - firstColon - 1);
    std::string expStr = basePart.substr(secondColon + 1);
    
    try {
        payload.user_id = std::stoi(userIdStr);
        payload.username = username;
        payload.exp = std::stoll(expStr);
        const long long now = static_cast<long long>(std::time(nullptr));
        payload.valid = (payload.exp > now);
    } catch (...) {
        return payload;
    }
    
    return payload;
}

std::string Auth::extractToken(const std::string& authHeader) {
    // Handle "Bearer <token>" format
    const std::string prefix = "Bearer ";
    if (authHeader.substr(0, prefix.size()) == prefix) {
        return authHeader.substr(prefix.size());
    }
    // If no Bearer prefix, assume the header is the token itself
    return authHeader;
}
