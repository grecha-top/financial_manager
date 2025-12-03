#include "BudgetController.h"
#include <drogon/HttpResponse.h>
#include <drogon/orm/CoroMapper.h>
#include <jsoncpp/json/json.h>
#include <drogon/HttpAppFramework.h>
#include "utils/JwtUtils.h"

using namespace finance;
using namespace drogon_model::financial_manager;
using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::Task;

Task<HttpResponsePtr> BudgetController::CreateBudget(HttpRequestPtr req) {
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

        if (!json->isMember("id_category") ||
            !json->isMember("month") ||
            !json->isMember("year") ||
            !json->isMember("limit_amount")) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Missing required fields: id_category, month, year, limit_amount");
            co_return resp;
        }

        Budgets b;
        b.setIdUser(static_cast<int32_t>(*userIdOpt));
        b.setIdCategory((*json)["id_category"].asInt());
        b.setMonth((*json)["month"].asInt());
        b.setYear((*json)["year"].asInt());
        b.setLimitAmount((*json)["limit_amount"].asString());

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Budgets> mapper(db);
        auto inserted = co_await mapper.insert(b);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(inserted.toJson());
        resp->setStatusCode(drogon::k201Created);
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "CreateBudget error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> BudgetController::GetBudgets(HttpRequestPtr req) {
    try {
        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Budgets> mapper(db);

        auto userIdOpt = jwt_utils::getUserIdFromRequest(req);
        if (!userIdOpt) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Unauthorized");
            co_return resp;
        }

        std::vector<Budgets> bs = co_await mapper.findBy(
            drogon::orm::Criteria(Budgets::Cols::_id_user,
                                  drogon::orm::CompareOperator::EQ,
                                  static_cast<int32_t>(*userIdOpt)));

        Json::Value arr(Json::arrayValue);
        for (const auto &b : bs) {
            arr.append(b.toJson());
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(arr);
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "GetBudgets error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> BudgetController::UpdateBudget(HttpRequestPtr req, int budgetId) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid JSON");
            co_return resp;
        }

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Budgets> mapper(db);
        auto b = co_await mapper.findByPrimaryKey(budgetId);

        if (json->isMember("id_category")) {
            b.setIdCategory((*json)["id_category"].asInt());
        }
        if (json->isMember("month")) {
            b.setMonth((*json)["month"].asInt());
        }
        if (json->isMember("year")) {
            b.setYear((*json)["year"].asInt());
        }
        if (json->isMember("limit_amount")) {
            b.setLimitAmount((*json)["limit_amount"].asString());
        }

        co_await mapper.update(b);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(b.toJson());
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Budget not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "UpdateBudget error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> BudgetController::DeleteBudget(HttpRequestPtr /*req*/, int budgetId) {
    try {
        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Budgets> mapper(db);
        co_await mapper.deleteByPrimaryKey(budgetId);

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Budget not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "DeleteBudget error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}


