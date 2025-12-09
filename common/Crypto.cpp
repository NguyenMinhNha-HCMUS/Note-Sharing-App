
#include "Crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <openssl/kdf.h>
//#include <openssl/pkcs5.h> // For PBKDF2
#include <iomanip>
#include <sstream>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <cmath>

// --- Helper Functions (Hex, Base64) ---

std::string Crypto::toHex(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const unsigned char byte : data) {
        ss << std::setw(2) << (int)byte;
    }
    return ss.str();
}

std::vector<unsigned char> Crypto::fromHex(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::string Crypto::base64Encode(const std::vector<unsigned char>& data) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    // Bỏ ký tự xuống dòng
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); 

    if (BIO_write(bio, data.data(), data.size()) <= 0) {
        BIO_free_all(bio);
        return "";
    }
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string encoded(bufferPtr->data, bufferPtr->length);
    
    BIO_free_all(bio);
    return encoded;
}

std::vector<unsigned char> Crypto::base64Decode(const std::string& encoded) {
    BIO *bio, *b64;
    size_t len = encoded.length();
    
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(encoded.data(), len);
    bio = BIO_push(b64, bio);

    // Bỏ ký tự xuống dòng
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); 

    // Tính toán kích thước tối đa (3/4 * len)
    size_t max_len = std::ceil(len / 4.0) * 3 + 1;
    std::vector<unsigned char> decoded_data(max_len);
    
    int decode_len = BIO_read(bio, decoded_data.data(), decoded_data.size());

    if (decode_len <= 0) {
        BIO_free_all(bio);
        return {};
    }
    
    decoded_data.resize(decode_len);
    BIO_free_all(bio);
    return decoded_data;
}


// --- Utility Functions ---

// Hàm tạo bytes ngẫu nhiên (dùng làm IV, FileKey, Salt, etc.)
std::vector<unsigned char> Crypto::generateRandomBytes(int length) {
    std::vector<unsigned char> buffer(length);
    if (RAND_bytes(buffer.data(), length) != 1) {
        std::cerr << "RAND_bytes failed\n";
        return {};
    }
    return buffer;
}

// --- Key Derivation ---

// Dùng PBKDF2-HMAC-SHA256 để tạo Master Key từ Mật khẩu và Salt
std::vector<unsigned char> Crypto::deriveKeyPBKDF2(
    const std::string& password, 
    const std::string& salt
) {
    const int key_len = 32; // 32 bytes (256 bits) key size
    const int iterations = 310000; // Số lần lặp được khuyến nghị

    std::vector<unsigned char> key(key_len);
    std::vector<unsigned char> salt_bytes = fromHex(salt);

    if (PKCS5_PBKDF2_HMAC(
        password.c_str(), password.length(),
        salt_bytes.data(), salt_bytes.size(),
        iterations, 
        EVP_sha256(),
        key_len,
        key.data()
    ) != 1) {
        std::cerr << "PBKDF2 key derivation failed\n";
        return {};
    }

    return key;
}

// --- AES-256-CBC Encryption/Decryption ---

// Hàm mã hóa AES-256-CBC
std::vector<unsigned char> Crypto::AESEncrypt(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    
    size_t max_len = plaintext.size() + EVP_MAX_BLOCK_LENGTH; 
    std::vector<unsigned char> ciphertext(max_len);

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return {};
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

// Hàm giải mã AES-256-CBC
std::vector<unsigned char> Crypto::AESDecrypt(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    
    size_t max_len = ciphertext.size(); 
    std::vector<unsigned char> plaintext(max_len);

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        return {};
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        // Lỗi giải mã có thể do key/IV sai hoặc dữ liệu đã bị thay đổi
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(plaintext_len);
    return plaintext;
}

// --- ECDH Key Exchange ---

// Tạo cặp khóa ECDH (sử dụng P-256)
DHKeyPair Crypto::generateECDHKeyPair() {
    DHKeyPair keyPair;
    EC_KEY *eckey = NULL;
    
    // --- Khai báo biến ở đầu hàm để tránh lỗi 'jump to label crosses initialization' ---
    const int curve_nid = NID_X9_62_prime256v1; // Khai báo hằng số
    const BIGNUM *priv_bn = NULL;
    size_t priv_size = 0;
    std::vector<unsigned char> priv_bin;

    const EC_GROUP *group = NULL;
    const EC_POINT *pub_point = NULL;
    size_t pub_size = 0;
    std::vector<unsigned char> pub_bin;
    // --- Kết thúc Khai báo biến ---
    

    eckey = EC_KEY_new_by_curve_name(curve_nid);
    if (!eckey) goto cleanup;

    if (1 != EC_KEY_generate_key(eckey)) goto cleanup;

    // Lấy Private Key (Hex String)
    priv_bn = EC_KEY_get0_private_key(eckey);
    if (!priv_bn) goto cleanup;

    priv_size = BN_num_bytes(priv_bn);
    priv_bin.resize(priv_size);
    if (priv_size != (size_t)BN_bn2bin(priv_bn, priv_bin.data())) goto cleanup;
    keyPair.privateKey = toHex(priv_bin);

    // Lấy Public Key (Hex String - UNCOMPRESSED)
    group = EC_KEY_get0_group(eckey);
    pub_point = EC_KEY_get0_public_key(eckey);
    
    pub_size = EC_POINT_point2oct(group, pub_point, 
                                             POINT_CONVERSION_UNCOMPRESSED, NULL, 0, NULL);
    
    pub_bin.resize(pub_size);
    if (pub_size != EC_POINT_point2oct(group, pub_point, 
                                       POINT_CONVERSION_UNCOMPRESSED, pub_bin.data(), pub_size, NULL)) goto cleanup;
    keyPair.publicKey = toHex(pub_bin);
    
cleanup:
    // Xóa khóa riêng tư và giải phóng EC_KEY
    if (eckey) {
        if (const BIGNUM *priv = EC_KEY_get0_private_key(eckey)) {
            BN_zero(const_cast<BIGNUM*>(priv));
        }
        EC_KEY_free(eckey);
    }
    
    if (keyPair.privateKey.empty() || keyPair.publicKey.empty()) {
        keyPair.privateKey = "";
        keyPair.publicKey = "";
    }
    
    return keyPair;
}

// Tính Shared Secret (Session Key)
std::vector<unsigned char> Crypto::computeECDHSecret(
        const std::string& myPrivateKeyHex,
        const std::string& peerPublicKeyHex
) {
    // --- Khai báo biến ở đầu hàm để tránh lỗi 'jump to label crosses initialization' ---
    // Session Key sẽ là 32 bytes (256 bits)
    std::vector<unsigned char> shared_secret(32); 
    EC_KEY *eckey = NULL;
    const EC_GROUP *group = NULL;
    BIGNUM *priv_bn = NULL;
    EC_POINT *pub_point = NULL;
    const int curve_nid = NID_X9_62_prime256v1; // Move constant initialization up
    
    std::vector<unsigned char> priv_bin; // Khai báo vector
    std::vector<unsigned char> pub_bin; // Khai báo vector
    int shared_len = 0; // Khai báo và khởi tạo giá trị mặc định
    // --- Kết thúc Khai báo biến ---

    // Khóa riêng của tôi
    priv_bin = fromHex(myPrivateKeyHex); // Chỉ là gán (assignment)
    if (priv_bin.empty()) goto cleanup;
    
    priv_bn = BN_new();
    if (NULL == BN_bin2bn(priv_bin.data(), priv_bin.size(), priv_bn)) goto cleanup; 

    // Khóa công khai của đối phương (dạng EC_POINT)
    pub_bin = fromHex(peerPublicKeyHex); // Chỉ là gán (assignment)
    if (pub_bin.empty()) goto cleanup;

    // Setup EC_KEY từ Private Key
    eckey = EC_KEY_new_by_curve_name(curve_nid);
    if (!eckey) goto cleanup;
    group = EC_KEY_get0_group(eckey);
    
    if (1 != EC_KEY_set_private_key(eckey, priv_bn)) goto cleanup;
    
    pub_point = EC_POINT_new(group);
    if (NULL == pub_point) goto cleanup;

    if (1 != EC_POINT_oct2point(group, pub_point, pub_bin.data(), pub_bin.size(), NULL)) goto cleanup;

    // Tính toán Shared Secret
    shared_len = ECDH_compute_key(shared_secret.data(), shared_secret.size(), 
                                      pub_point, eckey, NULL);

    if (shared_len <= 0) {
        shared_secret.clear();
    } else {
        shared_secret.resize(std::min((size_t)shared_len, (size_t)32));
        if (shared_secret.size() < 32) {
             // Thêm padding nếu cần (thường không xảy ra với P-256)
             shared_secret.resize(32, 0); 
        }
    }
    
cleanup:
    // Xóa khóa riêng tư và các biến quan trọng khỏi bộ nhớ
    if (priv_bn) BN_clear_free(priv_bn); 
    if (pub_point) EC_POINT_free(pub_point);
    if (eckey) {
        if (const BIGNUM *priv = EC_KEY_get0_private_key(eckey)) {
            BN_zero(const_cast<BIGNUM*>(priv));
        }
        EC_KEY_free(eckey);
    }
    
    return shared_secret;
}

// --- Key Wrapping/Unwrapping (Dùng AES-256-CBC) ---

// Key Wrapping: Mã hóa KeyA bằng KeyB (dùng cho MasterKey -> FileKey hoặc SessionKey -> FileKey)
std::string Crypto::wrapKey(
    const std::vector<unsigned char>& keyToWrap, 
    const std::vector<unsigned char>& wrappingKey
) {
    if (wrappingKey.size() != 32) {
        std::cerr << "Error: WrappingKey must be 32 bytes for AES-256.\n";
        return "";
    }
    if (keyToWrap.size() != 32) { 
        std::cerr << "Error: KeyToWrap must be 32 bytes.\n";
        return "";
    }

    // 1. Tạo IV ngẫu nhiên
    std::vector<unsigned char> iv = generateRandomBytes(16);
    if (iv.empty()) return "";

    // 2. Mã hóa keyToWrap bằng wrappingKey (sử dụng AES-256-CBC)
    std::vector<unsigned char> encryptedKey = AESEncrypt(keyToWrap, wrappingKey, iv);
    
    // Xóa keyToWrap khỏi RAM ngay lập tức
    // Lưu ý: const_cast có thể bị coi là không an toàn nhưng cần thiết để xóa key
    std::fill(const_cast<unsigned char*>(keyToWrap.data()), const_cast<unsigned char*>(keyToWrap.data()) + keyToWrap.size(), 0);

    if (encryptedKey.empty()) return "";

    // 3. Kết hợp IV (16 bytes) và Key đã mã hóa
    std::vector<unsigned char> wrappedData;
    wrappedData.insert(wrappedData.end(), iv.begin(), iv.end());
    wrappedData.insert(wrappedData.end(), encryptedKey.begin(), encryptedKey.end());
    
    // 4. Base64 encode 
    return base64Encode(wrappedData);
}

// Key Unwrapping: Giải mã KeyA bằng KeyB
std::vector<unsigned char> Crypto::unwrapKey(
    const std::string& wrappedKey, 
    const std::vector<unsigned char>& unwrappingKey
) {
    if (wrappedKey.empty()) return {};
    if (unwrappingKey.size() != 32) return {};

    // 1. Base64 decode
    std::vector<unsigned char> wrappedData = base64Decode(wrappedKey);
    
    if (wrappedData.size() < 16 + 16) return {};

    // 2. Tách IV và Key đã mã hóa
    std::vector<unsigned char> iv(wrappedData.begin(), wrappedData.begin() + 16);
    std::vector<unsigned char> encryptedKey(wrappedData.begin() + 16, wrappedData.end());

    // 3. Giải mã Key đã mã hóa
    std::vector<unsigned char> unwrappedKey = AESDecrypt(encryptedKey, unwrappingKey, iv);

    // 4. Kiểm tra kích thước Key (phải là 32 bytes)
    if (!unwrappedKey.empty() && unwrappedKey.size() != 32) {
         std::fill(unwrappedKey.begin(), unwrappedKey.end(), 0);
         return {};
    }
    
    return unwrappedKey;
}

