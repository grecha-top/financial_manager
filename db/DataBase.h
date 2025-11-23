#pragma once
#include <drogon/orm/DbClient.h>
#include <drogon/drogon.h>

namespace db {

inline drogon::orm::DbClientPtr getDbClient() {
    static auto client = drogon::app().getFastDbClient("default");
    return client;
}

}