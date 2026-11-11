// Module 1 routes: map data, route finding and road closures.
#include "aegis/api.hpp"
#include "graphs/graph.hpp"
#include <chrono>
#include <cmath>
#include <map>

namespace aegis {
namespace {

struct Location { int id; std::string name; double lat, lng; };

struct MapData {
    Graph graph;
    std::map<int, Location> locations;
};

std::string text(sqlite3_stmt* st, int col) {
    const unsigned char* t = sqlite3_column_text(st, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

// Rebuilds the graph from SQLite so every request sees the latest road closures.
MapData loadMap(Database& db) {
    MapData m;
    db.query("SELECT id,name,latitude,longitude FROM locations ORDER BY id", [&](sqlite3_stmt* st) {
        Location l{sqlite3_column_int(st, 0), text(st, 1), sqlite3_column_double(st, 2), sqlite3_column_double(st, 3)};
        m.locations[l.id] = l;
        m.graph.addNode(l.id);
    });
    db.query("SELECT id,from_id,to_id,distance_km,is_blocked FROM roads", [&](sqlite3_stmt* st) {
        m.graph.addEdge(sqlite3_column_int(st, 1), sqlite3_column_int(st, 2), sqlite3_column_double(st, 3),
                        sqlite3_column_int(st, 0), sqlite3_column_int(st, 4) != 0);
    });
    return m;
}

const char* ROADS_SQL =
    "SELECT r.id, r.from_id, r.to_id, f.name AS from_name, t.name AS to_name, "
    "f.latitude AS from_lat, f.longitude AS from_lng, t.latitude AS to_lat, t.longitude AS to_lng, "
    "r.distance_km, r.is_blocked FROM roads r "
    "JOIN locations f ON f.id = r.from_id JOIN locations t ON t.id = r.to_id ";

void logEvent(Database& db, const std::string& message) {
    db.execScript("INSERT INTO simulation_events (disaster_id, category, message) VALUES "
                  "((SELECT id FROM disasters WHERE status='active' ORDER BY id DESC LIMIT 1), 'road', " +
                  Database::quote(message) + ")");
}

std::string roadNames(Database& db, int roadId) {
    std::string names;
    db.query("SELECT f.name, t.name FROM roads r JOIN locations f ON f.id=r.from_id "
             "JOIN locations t ON t.id=r.to_id WHERE r.id=" + std::to_string(roadId),
             [&](sqlite3_stmt* st) { names = text(st, 0) + " to " + text(st, 1); });
    return names;
}

}  // namespace

void registerMapRoutes(App& app, Database& db) {
    CROW_ROUTE(app, "/api/map/locations")([&db] {
        try {
            return jsonOk(db.rows("SELECT id,name,type,latitude,longitude FROM locations ORDER BY id"));
        } catch (const std::exception& e) { return jsonError(500, e.what()); }
    });

    CROW_ROUTE(app, "/api/map/roads")([&db] {
        try {
            return jsonOk(db.rows(std::string(ROADS_SQL) + "ORDER BY r.id"));
        } catch (const std::exception& e) { return jsonError(500, e.what()); }
    });

    // Body: {"from": id, "to": id, "algorithm": "dijkstra" | "bfs" | "dfs"}
    CROW_ROUTE(app, "/api/map/route").methods("POST"_method)([&db](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return jsonError(400, "Request body must be valid JSON");
        int from, to;
        if (!jsonInt(body, "from", from) || !jsonInt(body, "to", to))
            return jsonError(400, "Fields 'from' and 'to' must be location ids");
        std::string algo = "dijkstra";
        if (body.has("algorithm") && !jsonString(body, "algorithm", algo))
            return jsonError(400, "Field 'algorithm' must be a string");
        if (algo != "dijkstra" && algo != "bfs" && algo != "dfs")
            return jsonError(400, "Algorithm must be dijkstra, bfs or dfs");
        if (from == to) return jsonError(400, "Source and destination must be different locations");
        try {
            MapData m = loadMap(db);
            if (!m.locations.count(from) || !m.locations.count(to))
                return jsonError(404, "Source or destination location does not exist");

            auto t0 = std::chrono::steady_clock::now();
            PathResult r = algo == "bfs" ? m.graph.bfs(from, to)
                         : algo == "dfs" ? m.graph.dfs(from, to)
                                         : m.graph.dijkstra(from, to);
            double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();

            crow::json::wvalue b;
            b["found"] = r.found;
            b["algorithm"] = r.algorithm;
            b["from"] = from;
            b["to"] = to;
            b["runtime_us"] = std::round(us * 10.0) / 10.0;
            std::vector<crow::json::wvalue> path, visited;
            for (int id : r.path) {
                const Location& l = m.locations[id];
                crow::json::wvalue p;
                p["id"] = l.id; p["name"] = l.name; p["latitude"] = l.lat; p["longitude"] = l.lng;
                path.push_back(std::move(p));
            }
            for (int id : r.visited) {
                crow::json::wvalue v;
                v["id"] = id; v["name"] = m.locations[id].name;
                visited.push_back(std::move(v));
            }
            b["path"] = crow::json::wvalue::list(path);
            b["visited"] = crow::json::wvalue::list(visited);
            b["visited_count"] = static_cast<int>(r.visited.size());
            if (r.found) {
                b["distance_km"] = std::round(r.distance * 100.0) / 100.0;
                b["hops"] = static_cast<int>(r.path.size()) - 1;
                b["message"] = "Route found";
            } else {
                b["distance_km"] = nullptr;
                b["hops"] = nullptr;
                b["message"] = "No route: " + m.locations[to].name + " cannot be reached from " +
                               m.locations[from].name + " with the current road closures.";
            }
            return jsonOk(std::move(b));
        } catch (const std::exception& e) { return jsonError(500, e.what()); }
    });

    // Body: {"from_id": id, "to_id": id, "distance_km": number}
    CROW_ROUTE(app, "/api/map/roads").methods("POST"_method)([&db](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return jsonError(400, "Request body must be valid JSON");
        int a, b;
        double km;
        if (!jsonInt(body, "from_id", a) || !jsonInt(body, "to_id", b) || !jsonNumber(body, "distance_km", km))
            return jsonError(400, "Fields 'from_id', 'to_id' and 'distance_km' are required");
        if (a == b) return jsonError(400, "A road must connect two different locations");
        if (km <= 0 || km > 500) return jsonError(400, "Distance must be greater than 0 and at most 500 km");
        try {
            if (db.scalarInt("SELECT COUNT(*) FROM locations WHERE id IN (" + std::to_string(a) + "," +
                             std::to_string(b) + ")") != 2)
                return jsonError(404, "One or both locations do not exist");
            if (db.scalarInt("SELECT COUNT(*) FROM roads WHERE (from_id=" + std::to_string(a) + " AND to_id=" +
                             std::to_string(b) + ") OR (from_id=" + std::to_string(b) + " AND to_id=" +
                             std::to_string(a) + ")") > 0)
                return jsonError(409, "These two locations are already connected by a road");
            db.execScript("INSERT INTO roads (from_id,to_id,distance_km) VALUES (" + std::to_string(a) + "," +
                          std::to_string(b) + "," + std::to_string(km) + ")");
            long long id = db.scalarInt("SELECT last_insert_rowid()");
            auto row = db.firstRow(std::string(ROADS_SQL) + "WHERE r.id=" + std::to_string(id));
            logEvent(db, "New road added: " + roadNames(db, static_cast<int>(id)));
            crow::response res(201, row);
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) { return jsonError(500, e.what()); }
    });

    // Body: {"road_id": id, "blocked": true | false}
    CROW_ROUTE(app, "/api/map/roads/condition").methods("PATCH"_method)([&db](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return jsonError(400, "Request body must be valid JSON");
        int roadId;
        bool blocked;
        if (!jsonInt(body, "road_id", roadId) || !jsonBool(body, "blocked", blocked))
            return jsonError(400, "Fields 'road_id' (number) and 'blocked' (true or false) are required");
        try {
            if (db.scalarInt("SELECT COUNT(*) FROM roads WHERE id=" + std::to_string(roadId)) == 0)
                return jsonError(404, "Road " + std::to_string(roadId) + " does not exist");
            db.execScript("UPDATE roads SET is_blocked=" + std::string(blocked ? "1" : "0") +
                          " WHERE id=" + std::to_string(roadId));
            logEvent(db, "Road " + roadNames(db, roadId) + (blocked ? " blocked" : " reopened"));
            return jsonOk(db.firstRow(std::string(ROADS_SQL) + "WHERE r.id=" + std::to_string(roadId)));
        } catch (const std::exception& e) { return jsonError(500, e.what()); }
    });
}

}  // namespace aegis
