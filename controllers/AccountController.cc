#include "AccountController.h"
#include <drogon/HttpResponse.h>
#include <drogon/orm/CoroMapper.h>
#include <jsoncpp/json/json.h>
#include <drogon/utils/Utilities.h>
#include <drogon/HttpViewData.h>
#include <optional>
#include <cstdlib>
#include "models/Account.h"
#include "utils/JwtUtils.h"


using namespace finance;
using namespace drogon_model::financial_manager;
using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::Task;

Task<HttpResponsePtr> AccountController::createAccount(
    HttpRequestPtr req) {

    try {
        auto userIdOpt = jwt_utils::getUserIdFromRequest(req);
        if (!userIdOpt) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Unauthorized");
            co_return resp;
        }
        // 1. Проверка JSON
        auto json = req->getJsonObject();
        if (!json) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid JSON");
            co_return resp;
        }

        // 2. Валидация полей
        if (!json->isMember("account_name") || 
            !json->isMember("account_type")) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Missing required fields: account_name, account_type");
            co_return resp;
        }

        std::string account_name = (*json)["account_name"].asString();
        std::string account_type = (*json)["account_type"].asString();
        std::string balance = "0";
        
        // Проверяем начальный баланс, если указан
        if (json->isMember("balance")) {
            balance = (*json)["balance"].asString();
            // Валидация баланса
            try {
                double balanceValue = std::stod(balance);
                if (balanceValue < 0) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("Balance cannot be negative");
                    co_return resp;
                }
            } catch (...) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody("Invalid balance format");
                co_return resp;
            }
        }

        // 3. Валидация типа счёта
        if (account_type != "cash" && account_type != "card" && account_type != "deposit") {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid account_type. Must be 'cash', 'card', or 'deposit'");
            co_return resp;
        }

        // 4. Создаём объект модели
        Account account;
        account.setIdUser(static_cast<int32_t>(*userIdOpt));
        account.setAccountType(account_type);
        account.setAccountName(account_name);
        account.setBalance(balance);

        // 5. Вставляем через ORM
        auto db = drogon::app().getFastDbClient();
        auto mapper = drogon::orm::CoroMapper<Account>(db);
        auto inserted = co_await mapper.insert(account);

        // 6. Формируем ответ
        Json::Value result;
        result["id"] = (Json::UInt64)inserted.getValueOfId();
        result["id_user"] = (Json::Int64)inserted.getValueOfIdUser();
        result["account_name"] = inserted.getValueOfAccountName();
        result["account_type"] = inserted.getValueOfAccountType();
        result["balance"] = inserted.getValueOfBalance();
        const trantor::Date &createdAt = inserted.getValueOfCreatedAt();
        std::time_t t = createdAt.secondsSinceEpoch();
        std::tm tm = *std::localtime(&t);  // localtime = местное время; gmtime = UTC

        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
        result["created_at"] = std::string(buf);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
        resp->setStatusCode(drogon::k201Created);
        co_return resp;

    } catch (const std::exception &e) {
        LOG_ERROR << "Account creation error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> AccountController::GetAccounts(HttpRequestPtr req) {
    try {
        auto db = drogon::app().getFastDbClient();
        auto mapper = drogon::orm::CoroMapper<Account>(db);

        auto userIdOpt = jwt_utils::getUserIdFromRequest(req);
        if (!userIdOpt) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Unauthorized");
            co_return resp;
        }

        std::vector<Account> accounts = co_await mapper.findBy(
            drogon::orm::Criteria(Account::Cols::_id_user,
                                  drogon::orm::CompareOperator::EQ,
                                  static_cast<int32_t>(*userIdOpt)));

        Json::Value arr(Json::arrayValue);
        for (const auto &acc : accounts) {
            arr.append(acc.toJson());
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(arr);
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "GetAccounts error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> AccountController::GetAccountById(
    HttpRequestPtr /*req*/, int accountId) {
    try {
        auto db = drogon::app().getFastDbClient();
        auto mapper = drogon::orm::CoroMapper<Account>(db);
        auto account = co_await mapper.findByPrimaryKey(accountId);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(account.toJson());
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Account not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "GetAccountById error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> AccountController::UpdateAccount(
    HttpRequestPtr req, int accountId) {
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
        auto mapper = drogon::orm::CoroMapper<Account>(db);
        auto account = co_await mapper.findByPrimaryKey(accountId);

        // Проверяем права доступа
        if (account.getValueOfIdUser() != static_cast<int32_t>(*userIdOpt)) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k403Forbidden);
            resp->setBody("Account does not belong to user");
            co_return resp;
        }

        if (json->isMember("account_name")) {
            account.setAccountName((*json)["account_name"].asString());
        }
        if (json->isMember("account_type")) {
            std::string account_type = (*json)["account_type"].asString();
            if (account_type != "cash" && account_type != "card" && account_type != "deposit") {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody("Invalid account_type. Must be 'cash', 'card', or 'deposit'");
                co_return resp;
            }
            account.setAccountType(account_type);
        }
        if (json->isMember("balance")) {
            std::string balance = (*json)["balance"].asString();
            try {
                double balanceValue = std::stod(balance);
                if (balanceValue < 0) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("Balance cannot be negative");
                    co_return resp;
                }
            } catch (...) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody("Invalid balance format");
                co_return resp;
            }
            account.setBalance(balance);
        }

        co_await mapper.update(account);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(account.toJson());
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Account not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "UpdateAccount error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> AccountController::DeleteAccount(
    HttpRequestPtr /*req*/, int accountId) {
    try {
        auto db = drogon::app().getFastDbClient();
        auto mapper = drogon::orm::CoroMapper<Account>(db);
        co_await mapper.deleteByPrimaryKey(accountId);

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Account not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "DeleteAccount error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

void AccountController::showCreateAccountForm(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)> &&callback) {
    drogon::HttpViewData data;
    if (req->getOptionalParameter<std::string>("created") == "1") {
        data["hasSuccess"] = true;
    }
    else {
        data["hasSuccess"] = false;
    }
    auto resp = drogon::HttpResponse::newHttpViewResponse("create_account.csp", data);
    callback(resp);
}
