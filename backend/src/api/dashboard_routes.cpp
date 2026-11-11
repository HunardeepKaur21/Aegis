#include "aegis/api.hpp"

namespace aegis {

crow::response jsonError(int status, const std::string& message) {
    crow::json::wvalue body;
    body["error"] = message;
    crow::response res(status, body);
    res.set_header("Content-Type", "application/json");
    return res;
}

crow::response jsonOk(crow::json::wvalue body) {
    crow::response res(200, body);
    res.set_header("Content-Type", "application/json");
    return res;
}

bool jsonNumber(const crow::json::rvalue& b, const char* key, double& out) {
    if (b.t() != crow::json::type::Object || !b.has(key)) return false;
    if (b[key].t() != crow::json::type::Number) return false;
    out = b[key].d();
    return true;
}
bool jsonInt(const crow::json::rvalue& b, const char* key, int& out) {
    double d;
    if (!jsonNumber(b, key, d) || d != static_cast<int>(d)) return false;
    out = static_cast<int>(d);
    return true;
}
bool jsonBool(const crow::json::rvalue& b, const char* key, bool& out) {
    if (b.t() != crow::json::type::Object || !b.has(key)) return false;
    auto t = b[key].t();
    if (t != crow::json::type::True && t != crow::json::type::False) return false;
    out = (t == crow::json::type::True);
    return true;
}
bool jsonString(const crow::json::rvalue& b, const char* key, std::string& out) {
    if (b.t() != crow::json::type::Object || !b.has(key)) return false;
    if (b[key].t() != crow::json::type::String) return false;
    out = b[key].s();
    return true;
}

void registerDashboardRoutes(App& app, Database& db) {
    CROW_ROUTE(app, "/api/health")([] {
        crow::json::wvalue b;
        b["status"] = "ok";
        b["service"] = "aegis-backend";
        return jsonOk(std::move(b));
    });

    // Every figure below is read from SQLite on each request.
    CROW_ROUTE(app, "/api/dashboard")([&db] {
        try {
            crow::json::wvalue b;
            b["scenario"] = db.firstRow(
                "SELECT id,name,type,severity,description,started_at FROM disasters "
                "WHERE status='active' ORDER BY id DESC LIMIT 1");
            b["affected_people"] = db.scalarInt(
                "SELECT COALESCE(SUM(affected_people),0) FROM disasters WHERE status='active'");
            b["requests"]["pending"]  = db.scalarInt("SELECT COUNT(*) FROM rescue_requests WHERE status='pending'");
            b["requests"]["assigned"] = db.scalarInt("SELECT COUNT(*) FROM rescue_requests WHERE status='assigned'");
            b["requests"]["resolved"] = db.scalarInt("SELECT COUNT(*) FROM rescue_requests WHERE status='resolved'");
            b["requests"]["critical"] = db.scalarInt(
                "SELECT COUNT(*) FROM rescue_requests WHERE status='pending' AND severity>=4");
            b["teams"]["available"] = db.scalarInt("SELECT COUNT(*) FROM rescue_teams WHERE status='available'");
            b["teams"]["deployed"]  = db.scalarInt("SELECT COUNT(*) FROM rescue_teams WHERE status='deployed'");
            b["teams"]["total"]     = db.scalarInt("SELECT COUNT(*) FROM rescue_teams");
            b["shelters"]["capacity"] = db.scalarInt("SELECT COALESCE(SUM(capacity),0) FROM shelters");
            b["shelters"]["occupied"] = db.scalarInt("SELECT COALESCE(SUM(occupied),0) FROM shelters");
            b["shelters"]["list"] = db.rows("SELECT id,name,capacity,occupied FROM shelters ORDER BY id");
            b["resources"] = db.rows(
                "SELECT category, COUNT(*) AS items, SUM(quantity) AS units FROM resources "
                "GROUP BY category ORDER BY category");
            b["events"] = db.rows(
                "SELECT id,category,message,created_at FROM simulation_events ORDER BY created_at DESC, id DESC LIMIT 8");
            b["roads"]["blocked"] = db.scalarInt("SELECT COUNT(*) FROM roads WHERE is_blocked=1");
            b["roads"]["total"]   = db.scalarInt("SELECT COUNT(*) FROM roads");
            return jsonOk(std::move(b));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });
}

}  // namespace aegis
