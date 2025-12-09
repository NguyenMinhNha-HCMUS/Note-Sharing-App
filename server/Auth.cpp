#include "Auth.h"
// Có thể include thư viện jwt-cpp nếu muốn xịn

std::string Auth::generateToken(int user_id, std::string username) {
    // Requirement:
    // 1. Generate a secure token (e.g., JWT) containing user_id and username.
    // 2. Sign the token with a secret key.
    // 3. Return the token string.
    return "";
}

int Auth::verifyToken(std::string token) {
    // Requirement:
    // 1. Verify the signature of the token.
    // 2. Check expiration.
    // 3. Extract and return user_id if valid.
    // 4. Return -1 if invalid.
    return -1;
}
