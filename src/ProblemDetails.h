#pragma once

#include <nlohmann/json.hpp>

namespace nl = nlohmann;

namespace ash
{

struct ProblemDetail
{
    std::string     type;
    std::string     title;
    std::uint32_t   status;
    std::string     detail;
    std::string     instance;
};

void from_json(const nl::json& j, ProblemDetail& pd)
{
    j["type"].get_to(pd.type);
    j["title"].get_to(pd.title);
    j["status"].get_to(pd.status);
    j["detail"].get_to(pd.detail);
    j["instance"].get_to(pd.instance);
}

void to_json(nl::json& j, const ProblemDetail& pd)
{
    j["type"] = pd.type;
    j["title"] = pd.title;
    j["status"] = pd.status;
    j["detail"] = pd.detail;
    j["instance"] = pd.instance;
}

} // namespace