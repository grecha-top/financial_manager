#include "CategoryController.h"
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

Task<HttpResponsePtr> CategoryController::CreateCategory(HttpRequestPtr req) {
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

        if (!json->isMember("name") ||
            !json->isMember("type")) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Missing required fields: name, type");
            co_return resp;
        }

        std::string name = (*json)["name"].asString();
        std::string type = (*json)["type"].asString(); // income/expense

        if (type != "income" && type != "expense") {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid type. Must be 'income' or 'expense'");
            co_return resp;
        }

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Category> mapper(db);

        Category cat;
        cat.setIdUser(static_cast<int32_t>(*userIdOpt));
        cat.setName(name);
        cat.setType(type);

        auto inserted = co_await mapper.insert(cat);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(inserted.toJson());
        resp->setStatusCode(drogon::k201Created);
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "CreateCategory error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> CategoryController::GetCategories(HttpRequestPtr req) {
    try {
        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Category> mapper(db);

        auto userIdOpt = jwt_utils::getUserIdFromRequest(req);
        if (!userIdOpt) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k401Unauthorized);
            resp->setBody("Unauthorized");
            co_return resp;
        }

        std::vector<Category> cats = co_await mapper.findBy(
            drogon::orm::Criteria(Category::Cols::_id_user,
                                  drogon::orm::CompareOperator::EQ,
                                  static_cast<int32_t>(*userIdOpt)));

        Json::Value arr(Json::arrayValue);
        for (const auto &c : cats) {
            arr.append(c.toJson());
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(arr);
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "GetCategories error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> CategoryController::UpdateCategory(HttpRequestPtr req, int categoryId) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid JSON");
            co_return resp;
        }

        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Category> mapper(db);

        auto cat = co_await mapper.findByPrimaryKey(categoryId);

        if (json->isMember("name")) {
            cat.setName((*json)["name"].asString());
        }
        if (json->isMember("type")) {
            std::string type = (*json)["type"].asString();
            if (type != "income" && type != "expense") {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k400BadRequest);
                resp->setBody("Invalid type. Must be 'income' or 'expense'");
                co_return resp;
            }
            cat.setType(type);
        }

        co_await mapper.update(cat);

        auto resp = drogon::HttpResponse::newHttpJsonResponse(cat.toJson());
        resp->setStatusCode(drogon::k200OK);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Category not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "UpdateCategory error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}

Task<HttpResponsePtr> CategoryController::DeleteCategory(HttpRequestPtr /*req*/, int categoryId) {
    try {
        auto db = drogon::app().getFastDbClient();
        drogon::orm::CoroMapper<Category> mapper(db);
        co_await mapper.deleteByPrimaryKey(categoryId);

        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        co_return resp;
    } catch (const drogon::orm::UnexpectedRows &) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        resp->setBody("Category not found");
        co_return resp;
    } catch (const std::exception &e) {
        LOG_ERROR << "DeleteCategory error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        co_return resp;
    }
}


