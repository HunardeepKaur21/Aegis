#include "aegis/api.hpp"
#include "graphs/graph.hpp"
#include "rescue/dp_optimizer.hpp"
#include "rescue/merge_sort.hpp"
#include "rescue/rescue_engine.hpp"
#include <chrono>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace aegis {
namespace {

struct LocInfo {
    int id;
    std::string name;
    double lat;
    double lng;
};

struct RescueMap {
    Graph graph;
    std::map<int, LocInfo> locations;
};

std::string colText(sqlite3_stmt* st, int col) {
    const unsigned char* t = sqlite3_column_text(st, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

RescueMap loadRescueMap(Database& db) {
    RescueMap m;
    db.query("SELECT id,name,latitude,longitude FROM locations ORDER BY id", [&](sqlite3_stmt* st) {
        LocInfo l{sqlite3_column_int(st, 0), colText(st, 1), sqlite3_column_double(st, 2), sqlite3_column_double(st, 3)};
        m.locations[l.id] = l;
        m.graph.addNode(l.id);
    });
    db.query("SELECT id,from_id,to_id,distance_km,is_blocked FROM roads", [&](sqlite3_stmt* st) {
        m.graph.addEdge(sqlite3_column_int(st, 1), sqlite3_column_int(st, 2), sqlite3_column_double(st, 3),
                        sqlite3_column_int(st, 0), sqlite3_column_int(st, 4) != 0);
    });
    return m;
}

void logSimulationEvent(Database& db, const std::string& category, const std::string& message) {
    db.execScript("INSERT INTO simulation_events (disaster_id, category, message) VALUES "
                  "((SELECT id FROM disasters WHERE status='active' ORDER BY id DESC LIMIT 1), " +
                  Database::quote(category) + ", " + Database::quote(message) + ")");
}

EmergencyRequest rowToRequest(sqlite3_stmt* st) {
    EmergencyRequest r;
    r.id = sqlite3_column_int(st, 0);
    r.disaster_id = sqlite3_column_int(st, 1);
    r.location_id = sqlite3_column_int(st, 2);
    r.location_name = colText(st, 3);
    r.victim_name = colText(st, 4);
    r.emergency_type = colText(st, 5);
    r.severity = colText(st, 6);
    r.severity_level = stringToSeverity(r.severity);
    r.affected_people = sqlite3_column_int(st, 7);
    r.description = colText(st, 8);
    r.required_assistance = colText(st, 9);
    r.status = colText(st, 10);
    r.assigned_team_id = sqlite3_column_type(st, 11) == SQLITE_NULL ? 0 : sqlite3_column_int(st, 11);
    r.assigned_team_name = colText(st, 12);
    r.created_at = colText(st, 13);
    r.waiting_minutes = sqlite3_column_double(st, 14);

    // Calculate priority score using C++ engine formula
    r.priority_score = RescueEngine::calculatePriorityScore(
        r.severity_level, r.affected_people, r.waiting_minutes, r.emergency_type);
    return r;
}

const char* REQUESTS_SELECT_SQL =
    "SELECT r.id, r.disaster_id, r.location_id, COALESCE(l.name, 'Unknown Location') AS location_name, "
    "r.victim_name, r.emergency_type, r.severity, r.affected_people, r.description, r.required_assistance, "
    "r.status, r.assigned_team_id, COALESCE(t.name, '') AS assigned_team_name, r.created_at, "
    "MAX(0.0, (strftime('%s', 'now') - strftime('%s', r.created_at)) / 60.0) AS waiting_minutes "
    "FROM rescue_requests r "
    "LEFT JOIN locations l ON l.id = r.location_id "
    "LEFT JOIN rescue_teams t ON t.id = r.assigned_team_id ";

crow::json::wvalue requestToJson(const EmergencyRequest& r) {
    crow::json::wvalue w;
    w["id"] = r.id;
    w["disaster_id"] = r.disaster_id;
    w["location_id"] = r.location_id;
    w["location_name"] = r.location_name;
    w["victim_name"] = r.victim_name;
    w["emergency_type"] = r.emergency_type;
    w["severity"] = r.severity;
    w["severity_level"] = r.severity_level;
    w["affected_people"] = r.affected_people;
    w["description"] = r.description;
    w["required_assistance"] = r.required_assistance;
    w["priority_score"] = r.priority_score;
    w["status"] = r.status;
    w["assigned_team_id"] = r.assigned_team_id == 0 ? nullptr : crow::json::wvalue(r.assigned_team_id);
    w["assigned_team_name"] = r.assigned_team_name;
    w["created_at"] = r.created_at;
    w["waiting_minutes"] = std::round(r.waiting_minutes * 10.0) / 10.0;
    return w;
}

// Check team eligibility based on emergency type
bool isTeamEligible(const std::string& teamType, const std::string& emergencyType) {
    if (emergencyType == "Medical") {
        return teamType == "medical" || teamType == "general" || teamType == "search";
    }
    if (emergencyType == "Fire") {
        return teamType == "fire" || teamType == "general";
    }
    if (emergencyType == "Flood") {
        return teamType == "flood" || teamType == "boat" || teamType == "general";
    }
    if (emergencyType == "Building Collapse" || emergencyType == "Accident") {
        return teamType == "fire" || teamType == "general" || teamType == "search";
    }
    return true; // General or other teams eligible
}

} // namespace

void registerRescueRoutes(App& app, Database& db) {
    // GET /api/requests - list requests with filtering and C++ merge sort
    CROW_ROUTE(app, "/api/requests")([&db](const crow::request& req) {
        try {
            std::vector<EmergencyRequest> requests;
            db.query(std::string(REQUESTS_SELECT_SQL) + "ORDER BY r.id", [&](sqlite3_stmt* st) {
                requests.push_back(rowToRequest(st));
            });

            // Filtering
            auto qSeverity = req.url_params.get("severity");
            auto qStatus = req.url_params.get("status");
            auto qType = req.url_params.get("emergency_type");
            auto qSearch = req.url_params.get("search");

            std::vector<EmergencyRequest> filtered;
            filtered.reserve(requests.size());

            for (const auto& r : requests) {
                if (qSeverity && std::string(qSeverity) != "All" && std::string(qSeverity) != "" &&
                    r.severity != qSeverity) {
                    continue;
                }
                if (qStatus && std::string(qStatus) != "All" && std::string(qStatus) != "" &&
                    r.status != qStatus) {
                    continue;
                }
                if (qType && std::string(qType) != "All" && std::string(qType) != "" &&
                    r.emergency_type != qType) {
                    continue;
                }
                if (qSearch && std::string(qSearch) != "") {
                    std::string s = qSearch;
                    auto findCase = [](const std::string& haystack, const std::string& needle) {
                        auto it = std::search(
                            haystack.begin(), haystack.end(), needle.begin(), needle.end(),
                            [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); });
                        return it != haystack.end();
                    };
                    bool matches = findCase(r.victim_name, s) ||
                                  findCase(r.location_name, s) ||
                                  findCase(r.description, s) ||
                                  (std::to_string(r.id) == s);
                    if (!matches) continue;
                }
                filtered.push_back(r);
            }

            // Sorting using C++ Merge Sort
            auto qSort = req.url_params.get("sort");
            auto qOrder = req.url_params.get("order");
            bool ascending = (qOrder && std::string(qOrder) == "asc");

            RequestSortField sortField = RequestSortField::PriorityScore;
            if (qSort) {
                std::string sf = qSort;
                if (sf == "time" || sf == "waiting") sortField = RequestSortField::WaitingTime;
                else if (sf == "severity") sortField = RequestSortField::Severity;
                else if (sf == "people") sortField = RequestSortField::AffectedPeople;
                else if (sf == "id") sortField = RequestSortField::Id;
            }

            sortRequests(filtered, sortField, ascending);

            std::vector<crow::json::wvalue> list;
            list.reserve(filtered.size());
            for (const auto& r : filtered) {
                list.push_back(requestToJson(r));
            }

            return jsonOk(crow::json::wvalue::list(list));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // POST /api/requests - create a new emergency request
    CROW_ROUTE(app, "/api/requests").methods("POST"_method)([&db](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return jsonError(400, "Request body must be valid JSON");

        std::string victimName, emergencyType, severity, description, assistance;
        int affectedPeople = 1, locationId = 0;

        if (!jsonString(body, "victim_name", victimName) || victimName.empty()) {
            return jsonError(400, "Field 'victim_name' is required");
        }
        if (!jsonString(body, "emergency_type", emergencyType) || emergencyType.empty()) {
            return jsonError(400, "Field 'emergency_type' is required");
        }
        if (!jsonString(body, "severity", severity) || severity.empty()) {
            return jsonError(400, "Field 'severity' is required");
        }
        if (!jsonInt(body, "affected_people", affectedPeople) || affectedPeople <= 0) {
            return jsonError(400, "Field 'affected_people' must be a positive integer");
        }
        if (!jsonInt(body, "location_id", locationId) || locationId <= 0) {
            return jsonError(400, "Field 'location_id' is required");
        }
        jsonString(body, "description", description);
        jsonString(body, "required_assistance", assistance);

        try {
            if (db.scalarInt("SELECT COUNT(*) FROM locations WHERE id=" + std::to_string(locationId)) == 0) {
                return jsonError(404, "Location with ID " + std::to_string(locationId) + " does not exist");
            }

            int severityLevel = stringToSeverity(severity);
            double priorityScore = RescueEngine::calculatePriorityScore(
                severityLevel, affectedPeople, 0.0, emergencyType);

            std::ostringstream sql;
            sql << "INSERT INTO rescue_requests (disaster_id, location_id, victim_name, requester_name, "
                << "emergency_type, severity, affected_people, people_affected, description, required_assistance, "
                << "medical_emergency, priority_score, urgency_score, status, created_at) VALUES ("
                << "1, " << locationId << ", "
                << Database::quote(victimName) << ", "
                << Database::quote(victimName) << ", "
                << Database::quote(emergencyType) << ", "
                << Database::quote(severity) << ", "
                << affectedPeople << ", " << affectedPeople << ", "
                << Database::quote(description) << ", "
                << Database::quote(assistance) << ", "
                << (emergencyType == "Medical" ? 1 : 0) << ", "
                << priorityScore << ", " << priorityScore << ", "
                << "'pending', datetime('now'))";

            db.execScript(sql.str());
            long long newId = db.scalarInt("SELECT last_insert_rowid()");

            logSimulationEvent(db, "rescue", "New emergency request #" + std::to_string(newId) +
                               " logged: " + emergencyType + " at location " + std::to_string(locationId) +
                               " (" + std::to_string(affectedPeople) + " affected)");

            // Fetch created request
            EmergencyRequest created;
            db.query(std::string(REQUESTS_SELECT_SQL) + "WHERE r.id=" + std::to_string(newId), [&](sqlite3_stmt* st) {
                created = rowToRequest(st);
            });

            crow::response res(201, requestToJson(created));
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // GET /api/requests/<int> - get single request with priority breakdown and assignment details
    CROW_ROUTE(app, "/api/requests/<int>")([&db](int id) {
        try {
            EmergencyRequest req;
            bool found = false;
            db.query(std::string(REQUESTS_SELECT_SQL) + "WHERE r.id=" + std::to_string(id), [&](sqlite3_stmt* st) {
                req = rowToRequest(st);
                found = true;
            });
            if (!found) {
                return jsonError(404, "Request #" + std::to_string(id) + " not found");
            }

            crow::json::wvalue body = requestToJson(req);

            // Detailed priority breakdown
            PriorityBreakdown b = RescueEngine::getPriorityBreakdown(
                req.severity_level, req.affected_people, req.waiting_minutes, req.emergency_type);
            body["priority_breakdown"]["severity_component"] = b.severity_component;
            body["priority_breakdown"]["people_component"] = b.people_component;
            body["priority_breakdown"]["waiting_component"] = b.waiting_component;
            body["priority_breakdown"]["type_bonus"] = b.type_bonus;
            body["priority_breakdown"]["total_score"] = b.total_score;

            // Route & Assignment details if assigned
            if (req.assigned_team_id > 0) {
                auto assignmentRow = db.firstRow(
                    "SELECT id, team_id, assigned_at, status, route_distance FROM rescue_assignments "
                    "WHERE request_id=" + std::to_string(id) + " ORDER BY id DESC LIMIT 1");
                body["assignment"] = std::move(assignmentRow);

                auto teamRow = db.firstRow(
                    "SELECT t.id, t.name, t.team_type, t.current_location_id, l.name AS location_name "
                    "FROM rescue_teams t LEFT JOIN locations l ON l.id = t.current_location_id "
                    "WHERE t.id=" + std::to_string(req.assigned_team_id));
                body["team_details"] = std::move(teamRow);
            }

            // Events history for this request
            body["history"] = db.rows(
                "SELECT id, category, message, created_at FROM simulation_events "
                "WHERE message LIKE '%#" + std::to_string(id) + "%' "
                "ORDER BY id DESC LIMIT 5");

            return jsonOk(std::move(body));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // PATCH /api/requests/<int>/status - update request status
    CROW_ROUTE(app, "/api/requests/<int>/status").methods("PATCH"_method)([&db](const crow::request& req, int id) {
        auto body = crow::json::load(req.body);
        if (!body) return jsonError(400, "Request body must be valid JSON");

        std::string newStatus;
        if (!jsonString(body, "status", newStatus)) {
            return jsonError(400, "Field 'status' must be a string");
        }
        if (newStatus != "pending" && newStatus != "assigned" &&
            newStatus != "in_progress" && newStatus != "completed") {
            return jsonError(400, "Invalid status. Must be pending, assigned, in_progress, or completed");
        }

        try {
            int count = db.scalarInt("SELECT COUNT(*) FROM rescue_requests WHERE id=" + std::to_string(id));
            if (count == 0) return jsonError(404, "Request #" + std::to_string(id) + " does not exist");

            int assignedTeamId = db.scalarInt(
                "SELECT COALESCE(assigned_team_id, 0) FROM rescue_requests WHERE id=" + std::to_string(id));

            db.execScript("UPDATE rescue_requests SET status=" + Database::quote(newStatus) +
                          (newStatus == "completed" ? ", urgency_score=0 " : " ") +
                          "WHERE id=" + std::to_string(id));

            if (newStatus == "completed" && assignedTeamId > 0) {
                // Free up team and complete assignment
                db.execScript("UPDATE rescue_teams SET status='available', availability='available' WHERE id=" +
                              std::to_string(assignedTeamId));
                db.execScript("UPDATE rescue_assignments SET status='completed' WHERE request_id=" +
                              std::to_string(id) + " AND team_id=" + std::to_string(assignedTeamId));
                logSimulationEvent(db, "rescue", "Request #" + std::to_string(id) +
                                   " completed. Team #" + std::to_string(assignedTeamId) + " returned to available.");
            } else {
                logSimulationEvent(db, "rescue", "Request #" + std::to_string(id) + " status updated to " + newStatus);
            }

            EmergencyRequest updated;
            db.query(std::string(REQUESTS_SELECT_SQL) + "WHERE r.id=" + std::to_string(id), [&](sqlite3_stmt* st) {
                updated = rowToRequest(st);
            });

            return jsonOk(requestToJson(updated));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // GET /api/teams - list rescue teams with locations, status, and capacities
    CROW_ROUTE(app, "/api/teams")([&db] {
        try {
            auto teams = db.rows(
                "SELECT t.id, t.name, t.team_type, t.capacity, t.availability, t.status, "
                "t.current_location_id, l.name AS location_name, "
                "r.id AS current_request_id, r.emergency_type AS current_emergency "
                "FROM rescue_teams t "
                "LEFT JOIN locations l ON l.id = t.current_location_id "
                "LEFT JOIN rescue_requests r ON r.assigned_team_id = t.id AND r.status IN ('assigned','in_progress') "
                "ORDER BY t.id");
            return jsonOk(std::move(teams));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // POST /api/requests/dispatch - find nearest available team using Graph Dijkstra and assign
    CROW_ROUTE(app, "/api/requests/dispatch").methods("POST"_method)([&db](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return jsonError(400, "Request body must be valid JSON");

        int requestId = 0;
        if (!jsonInt(body, "request_id", requestId) || requestId <= 0) {
            return jsonError(400, "Field 'request_id' is required");
        }

        try {
            EmergencyRequest emergency;
            bool foundReq = false;
            db.query(std::string(REQUESTS_SELECT_SQL) + "WHERE r.id=" + std::to_string(requestId), [&](sqlite3_stmt* st) {
                emergency = rowToRequest(st);
                foundReq = true;
            });

            if (!foundReq) {
                return jsonError(404, "Emergency request #" + std::to_string(requestId) + " not found");
            }
            if (emergency.status == "completed") {
                return jsonError(400, "Cannot dispatch team: emergency request #" + std::to_string(requestId) + " is already completed");
            }

            // Load road network and locations
            RescueMap m = loadRescueMap(db);
            if (!m.locations.count(emergency.location_id)) {
                return jsonError(404, "Emergency location does not exist in graph");
            }

            // Fetch available teams
            struct CandidateTeam {
                int id;
                std::string name;
                std::string team_type;
                int location_id;
                int capacity;
            };

            std::vector<CandidateTeam> availableTeams;
            int specificTeamId = 0;
            if (body.has("team_id")) jsonInt(body, "team_id", specificTeamId);

            std::string teamQuery = "SELECT id, name, team_type, current_location_id, capacity FROM rescue_teams "
                                    "WHERE availability='available' ";
            if (specificTeamId > 0) {
                teamQuery += "AND id=" + std::to_string(specificTeamId);
            }

            db.query(teamQuery, [&](sqlite3_stmt* st) {
                availableTeams.push_back({
                    sqlite3_column_int(st, 0),
                    colText(st, 1),
                    colText(st, 2),
                    sqlite3_column_int(st, 3),
                    sqlite3_column_int(st, 4)
                });
            });

            if (availableTeams.empty()) {
                if (specificTeamId > 0) {
                    return jsonError(400, "Requested team is currently unavailable or deployed.");
                }
                return jsonError(400, "No rescue teams are currently available. All units are deployed or offline.");
            }

            // Find nearest reachable team among eligible teams
            int bestTeamIndex = -1;
            PathResult bestRoute;
            double minDistance = 1e9;
            double bestRuntimeUs = 0.0;

            // First pass: try specialized eligible teams
            for (int pass = 0; pass < 2 && bestTeamIndex == -1; ++pass) {
                for (size_t i = 0; i < availableTeams.size(); ++i) {
                    const auto& team = availableTeams[i];
                    if (pass == 0 && !isTeamEligible(team.team_type, emergency.emergency_type)) {
                        continue; // Skip non-specialized in pass 0
                    }

                    auto t0 = std::chrono::steady_clock::now();
                    PathResult r = m.graph.dijkstra(team.location_id, emergency.location_id);
                    double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();

                    if (r.found && r.distance < minDistance) {
                        minDistance = r.distance;
                        bestTeamIndex = static_cast<int>(i);
                        bestRoute = r;
                        bestRuntimeUs = us;
                    }
                }
            }

            if (bestTeamIndex == -1) {
                return jsonError(400, "No route available to " + emergency.location_name +
                                      ". All access roads from available rescue teams are blocked by floodwater or debris.");
            }

            const auto& selectedTeam = availableTeams[bestTeamIndex];

            // Perform assignment in SQLite
            db.execScript("UPDATE rescue_requests SET assigned_team_id=" + std::to_string(selectedTeam.id) +
                          ", status='assigned' WHERE id=" + std::to_string(requestId));
            db.execScript("UPDATE rescue_teams SET status='deployed', availability='deployed' WHERE id=" +
                          std::to_string(selectedTeam.id));
            db.execScript("INSERT INTO rescue_assignments (request_id, team_id, route_distance, status) VALUES (" +
                          std::to_string(requestId) + ", " + std::to_string(selectedTeam.id) + ", " +
                          std::to_string(bestRoute.distance) + ", 'active')");

            logSimulationEvent(db, "rescue", selectedTeam.name + " dispatched to Request #" +
                               std::to_string(requestId) + " at " + emergency.location_name +
                               " (" + std::to_string(std::round(bestRoute.distance * 100.0) / 100.0) + " km)");

            // Build route response JSON
            crow::json::wvalue resp;
            resp["success"] = true;
            resp["request_id"] = requestId;
            resp["team"]["id"] = selectedTeam.id;
            resp["team"]["name"] = selectedTeam.name;
            resp["team"]["team_type"] = selectedTeam.team_type;
            resp["team"]["capacity"] = selectedTeam.capacity;
            resp["route"]["found"] = true;
            resp["route"]["distance_km"] = std::round(bestRoute.distance * 100.0) / 100.0;
            resp["route"]["hops"] = static_cast<int>(bestRoute.path.size()) - 1;
            resp["route"]["runtime_us"] = std::round(bestRuntimeUs * 10.0) / 10.0;

            std::vector<crow::json::wvalue> pathArray;
            for (int nodeId : bestRoute.path) {
                const auto& loc = m.locations[nodeId];
                crow::json::wvalue p;
                p["id"] = loc.id;
                p["name"] = loc.name;
                p["latitude"] = loc.lat;
                p["longitude"] = loc.lng;
                pathArray.push_back(std::move(p));
            }
            resp["route"]["path"] = crow::json::wvalue::list(pathArray);
            resp["message"] = selectedTeam.name + " dispatched successfully via optimal route (" +
                              std::to_string(std::round(bestRoute.distance * 10.0) / 10.0) + " km).";

            return jsonOk(std::move(resp));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // GET /api/rescue/overview - dashboard metrics for Rescue Requests
    CROW_ROUTE(app, "/api/rescue/overview")([&db] {
        try {
            crow::json::wvalue ov;
            ov["total"] = db.scalarInt("SELECT COUNT(*) FROM rescue_requests");
            ov["critical"] = db.scalarInt("SELECT COUNT(*) FROM rescue_requests WHERE severity='Critical'");
            ov["pending"] = db.scalarInt("SELECT COUNT(*) FROM rescue_requests WHERE status='pending'");
            ov["assigned"] = db.scalarInt("SELECT COUNT(*) FROM rescue_requests WHERE status IN ('assigned','in_progress')");
            ov["completed"] = db.scalarInt("SELECT COUNT(*) FROM rescue_requests WHERE status IN ('completed','resolved')");
            ov["available_teams"] = db.scalarInt("SELECT COUNT(*) FROM rescue_teams WHERE availability='available'");
            ov["total_teams"] = db.scalarInt("SELECT COUNT(*) FROM rescue_teams");
            return jsonOk(std::move(ov));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // POST /api/rescue/optimize - DP capacity allocation optimizer
    CROW_ROUTE(app, "/api/rescue/optimize").methods("POST"_method)([&db](const crow::request& req) {
        try {
            int capacity = 0;
            auto body = crow::json::load(req.body);
            if (body && body.has("capacity")) {
                jsonInt(body, "capacity", capacity);
            }
            if (capacity <= 0) {
                capacity = static_cast<int>(db.scalarInt(
                    "SELECT COALESCE(SUM(capacity), 25) FROM rescue_teams WHERE availability='available'"));
            }

            std::vector<EmergencyRequest> pendingRequests;
            db.query(std::string(REQUESTS_SELECT_SQL) + "WHERE r.status='pending' ORDER BY r.id", [&](sqlite3_stmt* st) {
                pendingRequests.push_back(rowToRequest(st));
            });

            AllocationResult opt = optimizeRescueAllocation(pendingRequests, capacity);

            crow::json::wvalue res;
            res["total_capacity"] = opt.total_capacity;
            res["used_capacity"] = opt.used_capacity;
            res["remaining_capacity"] = opt.remaining_capacity;
            res["total_priority_score"] = std::round(opt.total_priority_score * 10.0) / 10.0;
            res["total_people_saved"] = opt.total_people_saved;
            res["runtime_us"] = std::round(opt.runtime_us * 10.0) / 10.0;

            std::vector<crow::json::wvalue> selectedList;
            for (const auto& r : opt.selected_requests) {
                selectedList.push_back(requestToJson(r));
            }
            res["selected_requests"] = crow::json::wvalue::list(selectedList);
            res["selected_count"] = static_cast<int>(selectedList.size());
            res["pending_count"] = static_cast<int>(pendingRequests.size());

            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // GET /api/rescue/heap - current Max Heap structure for visualization
    CROW_ROUTE(app, "/api/rescue/heap")([&db] {
        try {
            std::vector<EmergencyRequest> pendingRequests;
            db.query(std::string(REQUESTS_SELECT_SQL) + "WHERE r.status='pending' ORDER BY r.id", [&](sqlite3_stmt* st) {
                pendingRequests.push_back(rowToRequest(st));
            });

            EmergencyPriorityQueue pq;
            for (const auto& r : pendingRequests) {
                pq.insert(r);
            }

            std::vector<crow::json::wvalue> heapNodes;
            for (const auto& r : pq.displayHeap()) {
                crow::json::wvalue node;
                node["id"] = r.id;
                node["victim_name"] = r.victim_name;
                node["emergency_type"] = r.emergency_type;
                node["severity"] = r.severity;
                node["priority_score"] = r.priority_score;
                node["affected_people"] = r.affected_people;
                node["location_name"] = r.location_name;
                heapNodes.push_back(std::move(node));
            }

            crow::json::wvalue res;
            res["heap"] = crow::json::wvalue::list(heapNodes);
            res["size"] = static_cast<int>(heapNodes.size());
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });
}

} // namespace aegis
