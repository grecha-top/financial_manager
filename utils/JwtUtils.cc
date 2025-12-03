#include <optional>
#include "JwtUtils.h"
#include <jwt-cpp/jwt.h>
#include <drogon/HttpAppFramework.h>

// ⚠️ Замените на значение из config.json в продакшене!
static const std::string JWT_SECRET = "your_strong_secret_key_123!_CHANGE_ME";

std::string jwt_utils::createToken(int64_t user_id, const std::string &email) {
    auto now = std::chrono::system_clock::now();
    return jwt::create()
        .set_issuer("financial_manager")
        .set_type("JWT")
        .set_payload_claim("user_id", jwt::claim(std::to_string(user_id)))
        .set_payload_claim("email", jwt::claim(email))
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::hours(24))
        .sign(jwt::algorithm::hs256{JWT_SECRET});
}

std::optional<int64_t> jwt_utils::getUserIdFromRequest(const drogon::HttpRequestPtr &req) {
    auto auth = req->getHeader("Authorization");
    if (auth.empty() || auth.substr(0, 7) != "Bearer ") return std::nullopt;
    
    try {
        auto token = auth.substr(7);
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{JWT_SECRET})
            .with_issuer("financial_manager");
        verifier.verify(decoded);
        
        return std::stoll(decoded.get_payload_claim("user_id").as_string());
    } catch (...) {
        return std::nullopt;
    }
}