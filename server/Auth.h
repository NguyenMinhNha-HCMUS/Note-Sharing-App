#pragma once
#include <string>

class Auth {
public:
    // Tạo JWT token từ user_id và username
    static std::string generateToken(int user_id, std::string username);
    
    // Kiểm tra token, trả về user_id. Trả về -1 nếu lỗi.
    static int verifyToken(std::string token);
};

