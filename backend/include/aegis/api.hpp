#pragma once
#include <crow.h>
#include "aegis/database.hpp"

namespace aegis {

// Adds permissive CORS headers so the React dev server can call the API.
struct CorsMiddleware {
    struct context {};
    void before_handle(crow::request&, crow::response&, context&) {}
    void after_handle(crow::request&, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    }
};

using App = crow::App<CorsMiddleware>;

crow::response jsonError(int status, const std::string& message);
crow::response jsonOk(crow::json::wvalue body);

// One function per route group. Each module adds its own file under src/api/.
void registerDashboardRoutes(App& app, Database& db);
void registerMapRoutes(App& app, Database& db);
void registerRescueRoutes(App& app, Database& db);
void registerResourceRoutes(App& app, Database& db);
void registerShelterRoutes(App& app, Database& db);
void registerLabRoutes(App& app, Database& db);

// Small JSON-body helpers shared by all route files. Return false if the key is missing
// or has the wrong type, so routes can answer 400 with a clear message.
bool jsonInt(const crow::json::rvalue& body, const char* key, int& out);
bool jsonNumber(const crow::json::rvalue& body, const char* key, double& out);
bool jsonBool(const crow::json::rvalue& body, const char* key, bool& out);
bool jsonString(const crow::json::rvalue& body, const char* key, std::string& out);

}  // namespace aegis
