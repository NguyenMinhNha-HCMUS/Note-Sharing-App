#include "test_framework.h"
#include "../server/Auth.h"
#include <ctime>

TEST(Auth_GenerateToken) {
    std::string token = Auth::generateToken(1, "testuser");
    ASSERT_FALSE(token.empty());
    
    // Token should contain a dot (separator between base and signature)
    ASSERT_TRUE(token.find('.') != std::string::npos);
    
    return true;
}

TEST(Auth_VerifyToken_Valid) {
    int userId = 42;
    std::string username = "testuser";
    
    std::string token = Auth::generateToken(userId, username);
    TokenPayload payload = Auth::verifyToken(token);
    
    ASSERT_TRUE(payload.valid);
    ASSERT_EQ(payload.user_id, userId);
    ASSERT_STREQ(payload.username.c_str(), username.c_str());
    ASSERT_TRUE(payload.exp > 0);
    
    return true;
}

TEST(Auth_VerifyToken_Invalid) {
    TokenPayload payload = Auth::verifyToken("invalid.token.here");
    ASSERT_FALSE(payload.valid);
    
    return true;
}

TEST(Auth_VerifyToken_Empty) {
    TokenPayload payload = Auth::verifyToken("");
    ASSERT_FALSE(payload.valid);
    
    return true;
}

TEST(Auth_VerifyToken_Modified) {
    std::string token = Auth::generateToken(1, "user");
    
    // Modify the token
    token[token.length() - 1] = 'X';
    
    TokenPayload payload = Auth::verifyToken(token);
    ASSERT_FALSE(payload.valid);
    
    return true;
}

TEST(Auth_ExtractToken_Bearer) {
    std::string header = "Bearer test_token_12345";
    std::string token = Auth::extractToken(header);
    ASSERT_STREQ(token.c_str(), "test_token_12345");
    
    return true;
}

TEST(Auth_ExtractToken_NoBearer) {
    std::string header = "test_token_12345";
    std::string token = Auth::extractToken(header);
    ASSERT_STREQ(token.c_str(), "test_token_12345");
    
    return true;
}

TEST(Auth_ExtractToken_Empty) {
    std::string token = Auth::extractToken("");
    ASSERT_TRUE(token.empty());
    
    return true;
}

TEST(Auth_TokenExpiry) {
    // Generate token
    std::string token = Auth::generateToken(1, "user");
    TokenPayload payload = Auth::verifyToken(token);
    
    ASSERT_TRUE(payload.valid);
    ASSERT_TRUE(payload.exp > static_cast<long long>(std::time(nullptr)));
    
    return true;
}

TEST(Auth_DifferentUsers) {
    std::string token1 = Auth::generateToken(1, "user1");
    std::string token2 = Auth::generateToken(2, "user2");
    
    TokenPayload p1 = Auth::verifyToken(token1);
    TokenPayload p2 = Auth::verifyToken(token2);
    
    ASSERT_TRUE(p1.valid);
    ASSERT_TRUE(p2.valid);
    ASSERT_NE(p1.user_id, p2.user_id);
    ASSERT_STREQ(p1.username.c_str(), "user1");
    ASSERT_STREQ(p2.username.c_str(), "user2");
    
    return true;
}

