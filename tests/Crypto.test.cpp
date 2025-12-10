#include "test_framework.h"
#include "../common/Crypto.h"
#include <vector>
#include <string>

TEST(Crypto_GenerateRandomBytes) {
    std::vector<unsigned char> bytes = Crypto::generateRandomBytes(32);
    ASSERT_EQ(bytes.size(), 32);
    
    // Generate again and verify they're different (very likely)
    std::vector<unsigned char> bytes2 = Crypto::generateRandomBytes(32);
    ASSERT_EQ(bytes2.size(), 32);
    
    // They should be different (probability of collision is negligible)
    bool different = false;
    for (size_t i = 0; i < bytes.size(); i++) {
        if (bytes[i] != bytes2[i]) {
            different = true;
            break;
        }
    }
    ASSERT_TRUE(different);
    
    return true;
}

TEST(Crypto_HashSHA256) {
    std::string input = "test_string";
    std::string hash1 = Crypto::hashSHA256(input);
    std::string hash2 = Crypto::hashSHA256(input);
    
    // Same input should produce same hash
    ASSERT_STREQ(hash1.c_str(), hash2.c_str());
    
    // Hash should not be empty
    ASSERT_FALSE(hash1.empty());
    
    // Hash should be different from input
    ASSERT_NE(hash1, input);
    
    return true;
}

TEST(Crypto_HashSHA256_DifferentInputs) {
    std::string hash1 = Crypto::hashSHA256("input1");
    std::string hash2 = Crypto::hashSHA256("input2");
    
    // Different inputs should produce different hashes
    ASSERT_NE(hash1, hash2);
    
    return true;
}

TEST(Crypto_ToHex) {
    std::vector<unsigned char> bytes = {0x00, 0xFF, 0x0A, 0x1B};
    std::string hex = Crypto::toHex(bytes);
    
    ASSERT_STREQ(hex.c_str(), "00ff0a1b");
    
    return true;
}

TEST(Crypto_FromHex) {
    std::string hex = "00ff0a1b";
    std::vector<unsigned char> bytes = Crypto::fromHex(hex);
    
    ASSERT_EQ(bytes.size(), 4);
    ASSERT_EQ(bytes[0], 0x00);
    ASSERT_EQ(bytes[1], 0xFF);
    ASSERT_EQ(bytes[2], 0x0A);
    ASSERT_EQ(bytes[3], 0x1B);
    
    return true;
}

TEST(Crypto_Base64EncodeDecode) {
    std::string original = "Hello, World!";
    std::vector<unsigned char> originalBytes(original.begin(), original.end());
    
    std::string encoded = Crypto::base64Encode(originalBytes);
    ASSERT_FALSE(encoded.empty());
    
    std::vector<unsigned char> decoded = Crypto::base64Decode(encoded);
    std::string decodedStr(decoded.begin(), decoded.end());
    
    ASSERT_STREQ(decodedStr.c_str(), original.c_str());
    
    return true;
}

TEST(Crypto_AES_EncryptDecrypt) {
    std::vector<unsigned char> key = Crypto::generateRandomBytes(32); // AES-256
    std::vector<unsigned char> iv = Crypto::generateRandomBytes(16);
    std::string plaintext = "This is a test message for encryption";
    std::vector<unsigned char> plainBytes(plaintext.begin(), plaintext.end());
    
    // Encrypt
    std::vector<unsigned char> ciphertext = Crypto::encryptAES(plainBytes, key, iv);
    ASSERT_FALSE(ciphertext.empty());
    
    // Ciphertext should be different from plaintext (can't use ASSERT_NE with vectors)
    bool isDifferent = (ciphertext != plainBytes);
    ASSERT_TRUE(isDifferent);
    
    // Decrypt
    std::vector<unsigned char> decrypted = Crypto::decryptAES(ciphertext, key, iv);
    std::string decryptedStr(decrypted.begin(), decrypted.end());
    
    ASSERT_STREQ(decryptedStr.c_str(), plaintext.c_str());
    
    return true;
}

TEST(Crypto_AES_WrongKey) {
    std::vector<unsigned char> key1 = Crypto::generateRandomBytes(32);
    std::vector<unsigned char> key2 = Crypto::generateRandomBytes(32);
    std::vector<unsigned char> iv = Crypto::generateRandomBytes(16);
    std::string plaintext = "Test message";
    std::vector<unsigned char> plainBytes(plaintext.begin(), plaintext.end());
    
    // Encrypt with key1
    std::vector<unsigned char> ciphertext = Crypto::encryptAES(plainBytes, key1, iv);
    
    // Try to decrypt with key2 (should fail or produce garbage)
    std::vector<unsigned char> decrypted = Crypto::decryptAES(ciphertext, key2, iv);
    std::string decryptedStr(decrypted.begin(), decrypted.end());
    
    // Should not match original (using manual comparison to avoid printing issues)
    bool matches = (decryptedStr == plaintext);
    ASSERT_FALSE(matches);
    
    return true;
}

