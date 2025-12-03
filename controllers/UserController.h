#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpBinder.h>
#include <drogon/orm/CoroMapper.h>
#include "models/Users.h"

namespace finance {

class UserController : public drogon::HttpController<UserController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(UserController::Register, "/api/auth/register", drogon::Post);
        ADD_METHOD_TO(UserController::Login, "/api/auth/login", drogon::Post);
        ADD_METHOD_TO(UserController::GetProfile, "/api/auth/profile", drogon::Get);
        ADD_METHOD_TO(UserController::UpdateProfile, "/api/auth/profile", drogon::Put);
        ADD_METHOD_TO(UserController::DeleteAccount, "/api/auth/account", drogon::Delete);
        ADD_METHOD_TO(UserController::ShowRegisterPage, "/auth/register", drogon::Get);
        ADD_METHOD_TO(UserController::ShowLoginPage, "/auth/login", drogon::Get);
        ADD_METHOD_TO(UserController::ShowLogoutPage, "/auth/logout", drogon::Get);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> Register(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> Login(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> GetProfile(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> UpdateProfile(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> DeleteAccount(drogon::HttpRequestPtr req);

    void ShowRegisterPage(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)> &&callback);
    void ShowLoginPage(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)> &&callback);
    void ShowLogoutPage(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)> &&callback);
};

}


