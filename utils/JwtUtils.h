#pragma once
#include <optional>
#include <string>
#include <drogon/HttpRequest.h>

namespace jwt_utils {
    std::string createToken(int64_t user_id, const std::string &email);
    std::optional<int64_t> getUserIdFromRequest(const drogon::HttpRequestPtr &req);
}