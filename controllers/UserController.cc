#include "UserController.h"
#include <drogon/HttpResponse.h>
#include <drogon/orm/CoroMapper.h>
#include <jsoncpp/json/json.h>
#include <drogon/HttpAppFramework.h>
#include <drogon/HttpViewData.h>
#include "utils/PasswordUtils.h"
#include "utils/JwtUtils.h"

using namespace finance;
using namespace drogon_model::financial_manager;

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::Task;

Task<HttpResponsePtr> UserController::Register(HttpRequestPtr req) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid JSON");
            co_return resp;
        }

        if (!json->isMember("name") ||
            !json->isMember("email") ||
            !json->isMember("password")) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Missing required fields: name, email, password");
            co_return resp;
        }

        std::string name = (*json)["name"].asString();
        std::string email = (*json)["email"].asString();
        std::string password = (*json)["password"].asString();

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Users> mapper(db);

        // Проверяем, что такого email ещё нет
        try {
            auto existing = co_await mapper.findOne(
                drogon::orm::Criteria(Users::Cols::_email,
                                      drogon::orm::CompareOperator::EQ,
                                      email));
            (void)existing;
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k409Conflict);
            resp->setBody("User with this email already exists");
            co_return resp;
        } catch (const drogon::orm::UnexpectedRows & ) {
            // no user, ok
        }

        std::string hashed = security::hashPassword(password);

        Users user;
        user.setName(name);
        user.setEmail(email);
        user.setHashedPassword(hashed);

        auto inserted = co_await mapper.insert(user);

        Json::Value result;
        result["id"] = inserted.getValueOfId();
        result["name"] = inserted.getValueOfName();
        result["email"] = inserted.getValueOfEmail();
        result["token"] = jwt_utils::createToken(inserted.getValueOfId(), inserted.getValueOfEmail());

        auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
        resp->setStatusCode(drogon::k201Created);
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "Register error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> UserController::Login(HttpRequestPtr req) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid JSON");
            co_return resp;
        }

        if (!json->isMember("email") || !json->isMember("password")) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Missing required fields: email, password");
            co_return resp;
        }

        std::string email = (*json)["email"].asString();
        std::string password = (*json)["password"].asString();

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Users> mapper(db);

        Users user;
        try {
            user = co_await mapper.findOne(
                drogon::orm::Criteria(Users::Cols::_email,
                                      drogon::orm::CompareOperator::EQ,
                                      email));
        } catch (const drogon::orm::UnexpectedRows & ) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Invalid credentials");
            co_return resp;
        }

        if (!security::verifyPassword(password, user.getValueOfHashedPassword())) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Invalid credentials");
            co_return resp;
        }

        Json::Value result;
        result["id"] = user.getValueOfId();
        result["name"] = user.getValueOfName();
        result["email"] = user.getValueOfEmail();
        result["token"] = jwt_utils::createToken(user.getValueOfId(), user.getValueOfEmail());

        auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "Login error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> UserController::GetProfile(HttpRequestPtr req) {
    try {
        auto userIdOpt = jwt_utils::getUserIdFromRequest(req);
        if (!userIdOpt) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Unauthorized");
            co_return resp;
        }

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Users> mapper(db);
        auto user = co_await mapper.findByPrimaryKey(static_cast<int32_t>(*userIdOpt));

        Json::Value result;
        result["id"] = user.getValueOfId();
        result["name"] = user.getValueOfName();
        result["email"] = user.getValueOfEmail();

        auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("User not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "GetProfile error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> UserController::UpdateProfile(HttpRequestPtr req) {
    try {
        auto userIdOpt = jwt_utils::getUserIdFromRequest(req);
        if (!userIdOpt) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Unauthorized");
            co_return resp;
        }

        auto json = req->getJsonObject();
        if (!json) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid JSON");
            co_return resp;
        }

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Users> mapper(db);
        auto user = co_await mapper.findByPrimaryKey(static_cast<int32_t>(*userIdOpt));

        if (json->isMember("name")) {
            user.setName((*json)["name"].asString());
        }
        if (json->isMember("email")) {
            user.setEmail((*json)["email"].asString());
        }
        if (json->isMember("password")) {
            user.setHashedPassword(security::hashPassword((*json)["password"].asString()));
        }

        co_await mapper.update(user);

        Json::Value result;
        result["id"] = user.getValueOfId();
        result["name"] = user.getValueOfName();
        result["email"] = user.getValueOfEmail();

        auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("User not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "UpdateProfile error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> UserController::DeleteAccount(HttpRequestPtr req) {
    try {
        auto userIdOpt = jwt_utils::getUserIdFromRequest(req);
        if (!userIdOpt) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Unauthorized");
            co_return resp;
        }

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Users> mapper(db);
        co_await mapper.deleteByPrimaryKey(static_cast<int32_t>(*userIdOpt));

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("User not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "DeleteAccount error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

void UserController::ShowRegisterPage(const drogon::HttpRequestPtr&,
                                      std::function<void(const drogon::HttpResponsePtr&)> &&callback) {
    drogon::HttpViewData data;
    auto resp = drogon::HttpResponse::newHttpViewResponse("auth_register.csp", data);
    callback(resp);
}

void UserController::ShowLoginPage(const drogon::HttpRequestPtr&,
                                   std::function<void(const drogon::HttpResponsePtr&)> &&callback) {
    drogon::HttpViewData data;
    auto resp = drogon::HttpResponse::newHttpViewResponse("auth_login.csp", data);
    callback(resp);
}

void UserController::ShowLogoutPage(const drogon::HttpRequestPtr&,
                                    std::function<void(const drogon::HttpResponsePtr&)> &&callback) {
    drogon::HttpViewData data;
    auto resp = drogon::HttpResponse::newHttpViewResponse("logout.csp", data);
    callback(resp);
}


