#include "Crypto.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/param_build.h>
#include <openssl/core_names.h>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <iostream>
#include <stdexcept>

std::string Crypto::hashSHA256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), 
           input.size(), hash);
    return toHex(std::vector<unsigned char>(hash, hash + SHA256_DIGEST_LENGTH));
}

std::vector<unsigned char> Crypto::generateRandomBytes(int length) {
    std::vector<unsigned char> buffer(length);
    if (RAND_bytes(buffer.data(), length) != 1) {
        std::cerr << "Error generating random bytes" << std::endl;
        return std::vector<unsigned char>();
    }
    return buffer;
}

std::string Crypto::toHex(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

std::vector<unsigned char> Crypto::fromHex(const std::string& hex) {
    std::vector<unsigned char> result;
    if (hex.length() % 2 != 0) {
        return result;
    }
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byteStr, nullptr, 16));
        result.push_back(byte);
    }
    return result;
}

std::string Crypto::base64Encode(const std::vector<unsigned char>& data) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    BIO_write(bio, data.data(), static_cast<int>(data.size()));
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    return result;
}

std::vector<unsigned char> Crypto::base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.size()));
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    std::vector<unsigned char> result(encoded.size());
    int decodedLen = BIO_read(bio, result.data(), static_cast<int>(encoded.size()));
    
    BIO_free_all(bio);
    
    if (decodedLen > 0) {
        result.resize(decodedLen);
    } else {
        result.clear();
    }
    return result;
}

std::vector<unsigned char> Crypto::encryptAES(
    const std::vector<unsigned char>& plaintext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return std::vector<unsigned char>();
    }
    
    std::vector<unsigned char> ciphertext(plaintext.size() + AES_BLOCK_SIZE);
    int len = 0;
    int ciphertextLen = 0;
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::vector<unsigned char>();
    }
    
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), 
                          static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::vector<unsigned char>();
    }
    ciphertextLen = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::vector<unsigned char>();
    }
    ciphertextLen += len;
    
    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(ciphertextLen);
    return ciphertext;
}

std::vector<unsigned char> Crypto::decryptAES(
    const std::vector<unsigned char>& ciphertext,
    const std::vector<unsigned char>& key,
    const std::vector<unsigned char>& iv
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return std::vector<unsigned char>();
    }
    
    std::vector<unsigned char> plaintext(ciphertext.size());
    int len = 0;
    int plaintextLen = 0;
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::vector<unsigned char>();
    }
    
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(),
                          static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::vector<unsigned char>();
    }
    plaintextLen = len;
    
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return std::vector<unsigned char>();
    }
    plaintextLen += len;
    
    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(plaintextLen);
    return plaintext;
}

std::vector<unsigned char> Crypto::deriveKeyPBKDF2(
    const std::string& password,
    const std::string& salt
) {
    const int iterations = 10000;
    const int keyLen = 32;
    std::vector<unsigned char> key(keyLen);
    
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                          reinterpret_cast<const unsigned char*>(salt.c_str()),
                          static_cast<int>(salt.size()),
                          iterations, EVP_sha256(), keyLen, key.data()) != 1) {
        std::cerr << "PBKDF2 derivation failed" << std::endl;
        return std::vector<unsigned char>();
    }
    return key;
}

DHKeyPair Crypto::generateECDHKeyPair() {
    DHKeyPair keyPair;
    
    // Generate EC key pair using P-256 curve
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if (!ctx) {
        return keyPair;
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return keyPair;
    }
    
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return keyPair;
    }
    
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return keyPair;
    }
    EVP_PKEY_CTX_free(ctx);
    
    // Extract private key using OpenSSL 3.x API
    BIGNUM* privBn = nullptr;
    if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_PRIV_KEY, &privBn) == 1) {
        keyPair.private_key.resize(32);
        int privLen = BN_bn2binpad(privBn, keyPair.private_key.data(), 32);
        if (privLen != 32) {
            keyPair.private_key.clear();
        }
        BN_free(privBn);
    }
    
    // Extract public key
    size_t pubLen = 0;
    if (EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_PUB_KEY, nullptr, 0, &pubLen) == 1) {
        keyPair.public_key.resize(pubLen);
        EVP_PKEY_get_octet_string_param(pkey, OSSL_PKEY_PARAM_PUB_KEY, 
                                         keyPair.public_key.data(), pubLen, &pubLen);
    }
    
    EVP_PKEY_free(pkey);
    return keyPair;
}

std::vector<unsigned char> Crypto::computeECDHSecret(
    const std::string& my_private_key_hex,
    const std::string& peer_public_key_hex
) {
    std::vector<unsigned char> privBytes = fromHex(my_private_key_hex);
    std::vector<unsigned char> pubBytes = fromHex(peer_public_key_hex);
    
    if (privBytes.empty() || pubBytes.empty()) {
        return std::vector<unsigned char>();
    }
    
    // Build my key from private key bytes
    OSSL_PARAM_BLD* bld = OSSL_PARAM_BLD_new();
    if (!bld) {
        return std::vector<unsigned char>();
    }
    
    BIGNUM* privBn = BN_bin2bn(privBytes.data(), static_cast<int>(privBytes.size()), nullptr);
    
    OSSL_PARAM_BLD_push_utf8_string(bld, OSSL_PKEY_PARAM_GROUP_NAME, "prime256v1", 0);
    OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_PRIV_KEY, privBn);
    
    OSSL_PARAM* params = OSSL_PARAM_BLD_to_param(bld);
    
    EVP_PKEY_CTX* keyCtx = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
    EVP_PKEY_fromdata_init(keyCtx);
    
    EVP_PKEY* myKey = nullptr;
    EVP_PKEY_fromdata(keyCtx, &myKey, EVP_PKEY_KEYPAIR, params);
    
    OSSL_PARAM_free(params);
    OSSL_PARAM_BLD_free(bld);
    BN_free(privBn);
    EVP_PKEY_CTX_free(keyCtx);
    
    if (!myKey) {
        return std::vector<unsigned char>();
    }
    
    // Build peer key from public key bytes
    OSSL_PARAM_BLD* peerBld = OSSL_PARAM_BLD_new();
    OSSL_PARAM_BLD_push_utf8_string(peerBld, OSSL_PKEY_PARAM_GROUP_NAME, "prime256v1", 0);
    OSSL_PARAM_BLD_push_octet_string(peerBld, OSSL_PKEY_PARAM_PUB_KEY, pubBytes.data(), pubBytes.size());
    
    OSSL_PARAM* peerParams = OSSL_PARAM_BLD_to_param(peerBld);
    
    EVP_PKEY_CTX* peerKeyCtx = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
    EVP_PKEY_fromdata_init(peerKeyCtx);
    
    EVP_PKEY* peerKey = nullptr;
    EVP_PKEY_fromdata(peerKeyCtx, &peerKey, EVP_PKEY_PUBLIC_KEY, peerParams);
    
    OSSL_PARAM_free(peerParams);
    OSSL_PARAM_BLD_free(peerBld);
    EVP_PKEY_CTX_free(peerKeyCtx);
    
    if (!peerKey) {
        EVP_PKEY_free(myKey);
        return std::vector<unsigned char>();
    }
    
    // Derive shared secret
    EVP_PKEY_CTX* deriveCtx = EVP_PKEY_CTX_new(myKey, nullptr);
    if (!deriveCtx || EVP_PKEY_derive_init(deriveCtx) <= 0) {
        EVP_PKEY_free(myKey);
        EVP_PKEY_free(peerKey);
        if (deriveCtx) EVP_PKEY_CTX_free(deriveCtx);
        return std::vector<unsigned char>();
    }
    
    if (EVP_PKEY_derive_set_peer(deriveCtx, peerKey) <= 0) {
        EVP_PKEY_CTX_free(deriveCtx);
        EVP_PKEY_free(myKey);
        EVP_PKEY_free(peerKey);
        return std::vector<unsigned char>();
    }
    
    size_t secretLen = 0;
    EVP_PKEY_derive(deriveCtx, nullptr, &secretLen);
    
    std::vector<unsigned char> secret(secretLen);
    EVP_PKEY_derive(deriveCtx, secret.data(), &secretLen);
    
    EVP_PKEY_CTX_free(deriveCtx);
    EVP_PKEY_free(myKey);
    EVP_PKEY_free(peerKey);
    
    // Hash the shared secret to get a 32-byte key
    std::string secretHex = toHex(secret);
    std::string hashedSecret = hashSHA256(secretHex);
    return fromHex(hashedSecret);
}
