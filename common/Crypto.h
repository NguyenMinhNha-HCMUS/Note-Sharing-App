#pragma once
#include <vector>
#include <string>
#include "Protocol.h"

struct DHKeyPair {
    std::vector<unsigned char> private_key; // 32 bytes
    std::vector<unsigned char> public_key;  // 32 bytes
};

class Crypto {
public:
    // Hàm mã hóa AES-256-CBC
    static std::vector<unsigned char> encryptAES(
        const std::vector<unsigned char>& plaintext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv
    );
    // Hàm giải mã AES-256-CBC
    static std::vector<unsigned char> decryptAES(
        const std::vector<unsigned char>& ciphertext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv
    );

    static DHKeyPair generateECDHKeyPair();

    static std::vector<unsigned char> computeECDHSecret(
            const std::string& my_private_key_hex,
            const std::string& peer_public_key_hex
    );
    // Hàm băm SHA-256
    static std::string hashSHA256(const std::string& input);

    // Tạo Key từ Mật khẩu + Salt (PBKDF2) - Chạy phía Client
    static std::vector<unsigned char> deriveKeyPBKDF2(
        const std::string& password, 
        const std::string& salt
    );
    // Hàm tạo chuỗi ngẫu nhiên (dùng làm IV, Salt, etc.)
    static std::vector<unsigned char> generateRandomBytes(int length);

    // Chuyển đổi qua lại Hex
    static std::string toHex(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> fromHex(const std::string& hex);

    static std::string base64Encode(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> base64Decode(const std::string& encoded);
};
