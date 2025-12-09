#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpBinder.h>
#include <drogon/orm/CoroMapper.h>
#include "models/Budgets.h"

namespace finance {

class BudgetController : public drogon::HttpController<BudgetController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(BudgetController::CreateBudget, "/budgets", drogon::Post);
        ADD_METHOD_TO(BudgetController::GetBudgets, "/budgets", drogon::Get);
        ADD_METHOD_TO(BudgetController::UpdateBudget, "/budgets/{1}", drogon::Put);
        ADD_METHOD_TO(BudgetController::DeleteBudget, "/budgets/{1}", drogon::Delete);
    METHOD_LIST_END

    drogon::Task<drogon::HttpResponsePtr> CreateBudget(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> GetBudgets(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> UpdateBudget(drogon::HttpRequestPtr req, int budgetId);
    drogon::Task<drogon::HttpResponsePtr> DeleteBudget(drogon::HttpRequestPtr req, int budgetId);
};

}