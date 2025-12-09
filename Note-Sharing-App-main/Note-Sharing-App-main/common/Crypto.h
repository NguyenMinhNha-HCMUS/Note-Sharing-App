#pragma once
#include <vector>
#include <string>
// Bỏ #include "Protocol.h" nếu không dùng trực tiếp các struct Protocol

struct DHKeyPair {
    std::string privateKey; // Hex String
    std::string publicKey;  // Hex String
};

class Crypto {
public:
    // Hàm mã hóa AES-256-CBC
    static std::vector<unsigned char> AESEncrypt(
        const std::vector<unsigned char>& plaintext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv
    );
    // Hàm giải mã AES-256-CBC
    static std::vector<unsigned char> AESDecrypt(
        const std::vector<unsigned char>& ciphertext,
        const std::vector<unsigned char>& key,
        const std::vector<unsigned char>& iv
    );

    // Tạo cặp khóa ECDH
    static DHKeyPair generateECDHKeyPair();

    // Tính Session Key
    static std::vector<unsigned char> computeECDHSecret(
            const std::string& myPrivateKeyHex,
            const std::string& peerPublicKeyHex
    );
    
    // Tạo Master Key từ Mật khẩu + Salt (PBKDF2)
    static std::vector<unsigned char> deriveKeyPBKDF2(
        const std::string& password, 
        const std::string& salt
    );
    
    // Hàm tạo bytes ngẫu nhiên (dùng làm IV, FileKey, Salt, etc.)
    static std::vector<unsigned char> generateRandomBytes(int length);

    // Key Wrapping: Mã hóa KeyA bằng KeyB (dùng cho MasterKey -> FileKey hoặc SessionKey -> FileKey)
    static std::string wrapKey(
        const std::vector<unsigned char>& keyToWrap, 
        const std::vector<unsigned char>& wrappingKey
    );
    
    // Key Unwrapping: Giải mã KeyA bằng KeyB
    static std::vector<unsigned char> unwrapKey(
        const std::string& wrappedKey, 
        const std::vector<unsigned char>& unwrappingKey
    );
    
    // Chuyển đổi qua lại Hex
    static std::string toHex(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> fromHex(const std::string& hex);

    // Chuyển đổi qua lại Base64 (dùng cho Content và IV)
    static std::string base64Encode(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> base64Decode(const std::string& encoded);
};