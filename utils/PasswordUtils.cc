#include "PasswordUtils.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstring>
#include <stdexcept>
#include <iomanip>
#include <sstream>

static std::string bytesToHex(const unsigned char *bytes, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return ss.str();
}

static std::string generateSalt() {
    unsigned char salt[16];
    if (!RAND_bytes(salt, 16)) {
        throw std::runtime_error("Failed to generate salt");
    }
    return bytesToHex(salt, 16);
}

std::string security::hashPassword(const std::string &password) {
    std::string salt = generateSalt();
    const unsigned char *pass = reinterpret_cast<const unsigned char *>(password.c_str());
    unsigned char hash[32];

    if (!PKCS5_PBKDF2_HMAC(
        reinterpret_cast<const char *>(pass), password.size(),
        reinterpret_cast<const unsigned char *>(salt.c_str()), salt.size(),
        100000, EVP_sha256(), 32, hash)) {
        throw std::runtime_error("PBKDF2 failed");
    }

    return salt + "$" + bytesToHex(hash, 32);
}

bool security::verifyPassword(const std::string &password, const std::string &hash) {
    size_t pos = hash.find('$');
    if (pos == std::string::npos) return false;
    std::string salt = hash.substr(0, pos);
    std::string storedHash = hash.substr(pos + 1);

    const unsigned char *pass = reinterpret_cast<const unsigned char *>(password.c_str());
    unsigned char out[32];

    if (!PKCS5_PBKDF2_HMAC(
        reinterpret_cast<const char *>(pass), password.size(),
        reinterpret_cast<const unsigned char *>(salt.c_str()), salt.size(),
        100000, EVP_sha256(), 32, out)) {
        return false;
    }

    std::string computedHex = bytesToHex(out, 32);
    return storedHash == computedHex;
}