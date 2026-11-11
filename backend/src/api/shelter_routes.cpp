#include "aegis/api.hpp"
#include "graphs/graph.hpp"
#include "shelter/victim_types.hpp"
#include "shelter/victim_linked_list.hpp"
#include "shelter/victim_bst.hpp"
#include "shelter/victim_avl.hpp"
#include "shelter/victim_hash_map.hpp"
#include "shelter/shelter_recommender.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace aegis {
namespace {

std::string colText(sqlite3_stmt* st, int col) {
    const unsigned char* t = sqlite3_column_text(st, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

void logShelterEvent(Database& db, const std::string& message) {
    db.execScript("INSERT INTO simulation_events (disaster_id, category, message) VALUES "
                  "((SELECT id FROM disasters WHERE status='active' ORDER BY id DESC LIMIT 1), 'shelter', " +
                  Database::quote(message) + ")");
}

struct GraphData {
    Graph graph;
    std::map<int, std::string> locationNames;
};

GraphData loadRoadGraph(Database& db) {
    GraphData gd;
    db.query("SELECT id, name FROM locations ORDER BY id", [&](sqlite3_stmt* st) {
        int id = sqlite3_column_int(st, 0);
        std::string name = colText(st, 1);
        gd.locationNames[id] = name;
        gd.graph.addNode(id);
    });
    db.query("SELECT id, from_id, to_id, distance_km, is_blocked FROM roads", [&](sqlite3_stmt* st) {
        gd.graph.addEdge(sqlite3_column_int(st, 1), sqlite3_column_int(st, 2), sqlite3_column_double(st, 3),
                         sqlite3_column_int(st, 0), sqlite3_column_int(st, 4) != 0);
    });
    return gd;
}

VictimRecord rowToVictim(sqlite3_stmt* st) {
    VictimRecord v;
    v.id = sqlite3_column_int(st, 0);
    v.name = colText(st, 1);
    v.age = sqlite3_column_int(st, 2);
    v.gender = colText(st, 3);
    v.health_status = colText(st, 4);
    v.accompanying_people = sqlite3_column_int(st, 5);
    v.emergency_category = colText(st, 6);
    v.current_location_id = sqlite3_column_int(st, 7);
    v.location_name = colText(st, 8);
    v.medical_assistance = (sqlite3_column_int(st, 9) != 0);
    v.assigned_shelter_id = sqlite3_column_int(st, 10);
    v.assigned_shelter_name = colText(st, 11);
    v.status = colText(st, 12);
    v.created_at = colText(st, 13);
    return v;
}

crow::json::wvalue victimToJson(const VictimRecord& v) {
    crow::json::wvalue j;
    j["id"] = v.id;
    j["name"] = v.name;
    j["age"] = v.age;
    j["gender"] = v.gender;
    j["health_status"] = v.health_status;
    j["accompanying_people"] = v.accompanying_people;
    j["total_people"] = 1 + v.accompanying_people;
    j["emergency_category"] = v.emergency_category;
    j["current_location_id"] = v.current_location_id;
    j["location_name"] = v.location_name.empty() ? ("Location #" + std::to_string(v.current_location_id)) : v.location_name;
    j["medical_assistance"] = v.medical_assistance;
    if (v.assigned_shelter_id > 0) {
        j["assigned_shelter_id"] = v.assigned_shelter_id;
        j["assigned_shelter_name"] = v.assigned_shelter_name;
    } else {
        j["assigned_shelter_id"] = nullptr;
        j["assigned_shelter_name"] = "Awaiting Shelter";
    }
    j["status"] = v.status;
    j["created_at"] = v.created_at;
    return j;
}

ShelterInfo rowToShelter(sqlite3_stmt* st) {
    ShelterInfo s;
    s.id = sqlite3_column_int(st, 0);
    s.name = colText(st, 1);
    s.location_id = sqlite3_column_int(st, 2);
    s.location_name = colText(st, 3);
    s.total_capacity = sqlite3_column_int(st, 4);
    s.current_occupancy = sqlite3_column_int(st, 5);
    s.available_capacity = std::max(0, s.total_capacity - s.current_occupancy);
    s.has_medical = (sqlite3_column_int(st, 6) != 0);
    s.facilities = colText(st, 7);
    s.status = colText(st, 8);
    s.occupancy_pct = (s.total_capacity > 0) ? (static_cast<double>(s.current_occupancy) / s.total_capacity * 100.0) : 0.0;
    return s;
}

crow::json::wvalue shelterToJson(const ShelterInfo& s) {
    crow::json::wvalue j;
    j["id"] = s.id;
    j["name"] = s.name;
    j["location_id"] = s.location_id;
    j["location_name"] = s.location_name.empty() ? ("Location #" + std::to_string(s.location_id)) : s.location_name;
    j["total_capacity"] = s.total_capacity;
    j["capacity"] = s.total_capacity;
    j["current_occupancy"] = s.current_occupancy;
    j["occupied"] = s.current_occupancy;
    j["available_capacity"] = s.available_capacity;
    j["has_medical"] = s.has_medical;
    j["facilities"] = s.facilities;
    j["status"] = s.status;
    j["occupancy_pct"] = s.occupancy_pct;
    return j;
}

std::string computeShelterStatus(int occupied, int capacity) {
    if (occupied >= capacity) return "Full";
    if (occupied >= capacity * 0.9) return "Limited Capacity";
    return "Available";
}

} // namespace

void registerShelterRoutes(App& app, Database& db) {
    // -------------------------------------------------------------
    // GET /api/victims - list all victims with sorting and filtering
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/victims")([&db](const crow::request& req) {
        try {
            std::string category = req.url_params.get("category") ? req.url_params.get("category") : "All";
            std::string status = req.url_params.get("status") ? req.url_params.get("status") : "All";
            std::string search = req.url_params.get("search") ? req.url_params.get("search") : "";

            std::string sql = 
                "SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people, "
                "v.emergency_category, v.current_location_id, COALESCE(l.name, ''), v.medical_assistance, "
                "COALESCE(v.assigned_shelter_id, 0), COALESCE(s.name, ''), v.status, v.created_at "
                "FROM victims v "
                "LEFT JOIN locations l ON v.current_location_id = l.id "
                "LEFT JOIN shelters s ON v.assigned_shelter_id = s.id "
                "ORDER BY v.id DESC";

            // Load into a C++ Singly Linked List to demonstrate DSA collection traversal
            SinglyLinkedList<VictimRecord> victimList;
            db.query(sql, [&](sqlite3_stmt* st) {
                victimList.insertTail(rowToVictim(st));
            });

            // Convert to vector and apply in-memory filters / string matching
            std::vector<VictimRecord> allVictims = victimList.toVector();
            std::vector<crow::json::wvalue> filteredList;

            std::string searchLower = toLower(search);

            for (const auto& v : allVictims) {
                if (category != "All" && toLower(v.emergency_category) != toLower(category)) {
                    continue;
                }
                if (status != "All" && toLower(v.status) != toLower(status)) {
                    continue;
                }
                if (!searchLower.empty()) {
                    std::string idStr = std::to_string(v.id);
                    std::string nameLower = toLower(v.name);
                    if (idStr.find(searchLower) == std::string::npos && nameLower.find(searchLower) == std::string::npos) {
                        continue;
                    }
                }
                filteredList.push_back(victimToJson(v));
            }

            crow::json::wvalue res;
            res = std::move(filteredList);
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // POST /api/victims - register a new victim
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/victims").methods(crow::HTTPMethod::Post)([&db](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            std::string name;
            if (!jsonString(body, "name", name) || name.empty()) {
                return jsonError(400, "Missing or empty victim 'name'");
            }

            int age = 0;
            if (!jsonInt(body, "age", age) || age < 0 || age > 120) {
                return jsonError(400, "Invalid or missing 'age' (must be 0 - 120)");
            }

            int accompanying_people = 0;
            jsonInt(body, "accompanying_people", accompanying_people);
            if (accompanying_people < 0) accompanying_people = 0;

            std::string emergency_category = "Medium";
            jsonString(body, "emergency_category", emergency_category);

            std::string gender = "O";
            jsonString(body, "gender", gender);

            std::string health_status = "stable";
            jsonString(body, "health_status", health_status);

            int current_location_id = 1;
            jsonInt(body, "current_location_id", current_location_id);

            bool medical_assistance = false;
            jsonBool(body, "medical_assistance", medical_assistance);

            int assigned_shelter_id = 0;
            jsonInt(body, "assigned_shelter_id", assigned_shelter_id);

            std::string status = (assigned_shelter_id > 0) ? "sheltered" : "awaiting_shelter";

            // If a shelter was specified, verify capacity
            if (assigned_shelter_id > 0) {
                int total_needed = 1 + accompanying_people;
                long long avail = db.scalarInt(
                    "SELECT (total_capacity - current_occupancy) FROM shelters WHERE id = " + std::to_string(assigned_shelter_id));
                if (avail < total_needed) {
                    return jsonError(400, "Selected shelter does not have enough capacity for " + std::to_string(total_needed) + " people");
                }
            }

            std::string shelterSql = (assigned_shelter_id > 0) ? std::to_string(assigned_shelter_id) : "NULL";

            std::ostringstream sql;
            sql << "INSERT INTO victims (name, age, gender, health_status, accompanying_people, emergency_category, current_location_id, medical_assistance, shelter_id, assigned_shelter_id, status) VALUES ("
                << Database::quote(name) << ", "
                << age << ", "
                << Database::quote(gender) << ", "
                << Database::quote(health_status) << ", "
                << accompanying_people << ", "
                << Database::quote(emergency_category) << ", "
                << current_location_id << ", "
                << (medical_assistance ? 1 : 0) << ", "
                << shelterSql << ", "
                << shelterSql << ", "
                << Database::quote(status) << ")";
            db.execScript(sql.str());

            long long newId = db.scalarInt("SELECT last_insert_rowid()");

            // If assigned, increment shelter occupancy
            if (assigned_shelter_id > 0) {
                int people_count = 1 + accompanying_people;
                db.execScript(
                    "UPDATE shelters SET "
                    "current_occupancy = current_occupancy + " + std::to_string(people_count) + ", "
                    "occupied = occupied + " + std::to_string(people_count) + " "
                    "WHERE id = " + std::to_string(assigned_shelter_id));

                // Recalculate status
                db.execScript(
                    "UPDATE shelters SET status = CASE "
                    "WHEN current_occupancy >= total_capacity THEN 'Full' "
                    "WHEN current_occupancy >= total_capacity * 0.9 THEN 'Limited Capacity' "
                    "ELSE 'Available' END WHERE id = " + std::to_string(assigned_shelter_id));
            }

            logShelterEvent(db, "Registered victim #" + std::to_string(newId) + " (" + name + 
                                ", accompanying: " + std::to_string(accompanying_people) + ")");

            VictimRecord created;
            db.query(
                "SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people, "
                "v.emergency_category, v.current_location_id, COALESCE(l.name, ''), v.medical_assistance, "
                "COALESCE(v.assigned_shelter_id, 0), COALESCE(s.name, ''), v.status, v.created_at "
                "FROM victims v LEFT JOIN locations l ON v.current_location_id = l.id "
                "LEFT JOIN shelters s ON v.assigned_shelter_id = s.id WHERE v.id = " + std::to_string(newId),
                [&created](sqlite3_stmt* st) {
                    created = rowToVictim(st);
                });

            crow::response res(201, victimToJson(created));
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // GET /api/victims/<int> - get single victim details
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/victims/<int>")([&db](int id) {
        try {
            VictimRecord v;
            bool found = false;
            db.query(
                "SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people, "
                "v.emergency_category, v.current_location_id, COALESCE(l.name, ''), v.medical_assistance, "
                "COALESCE(v.assigned_shelter_id, 0), COALESCE(s.name, ''), v.status, v.created_at "
                "FROM victims v LEFT JOIN locations l ON v.current_location_id = l.id "
                "LEFT JOIN shelters s ON v.assigned_shelter_id = s.id WHERE v.id = " + std::to_string(id),
                [&](sqlite3_stmt* st) {
                    v = rowToVictim(st);
                    found = true;
                });

            if (!found) return jsonError(404, "Victim not found");
            return jsonOk(victimToJson(v));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // PUT /api/victims/<int> - update victim record
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/victims/<int>").methods(crow::HTTPMethod::Put)([&db](const crow::request& req, int id) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            long long exists = db.scalarInt("SELECT COUNT(*) FROM victims WHERE id = " + std::to_string(id));
            if (exists == 0) return jsonError(404, "Victim not found");

            std::string name;
            if (!jsonString(body, "name", name) || name.empty()) {
                return jsonError(400, "Missing or empty victim 'name'");
            }

            int age = 0;
            if (!jsonInt(body, "age", age) || age < 0 || age > 120) {
                return jsonError(400, "Invalid 'age'");
            }

            int accompanying_people = 0;
            jsonInt(body, "accompanying_people", accompanying_people);
            if (accompanying_people < 0) accompanying_people = 0;

            std::string emergency_category = "Medium";
            jsonString(body, "emergency_category", emergency_category);

            std::string health_status = "stable";
            jsonString(body, "health_status", health_status);

            int current_location_id = 1;
            jsonInt(body, "current_location_id", current_location_id);

            bool medical_assistance = false;
            jsonBool(body, "medical_assistance", medical_assistance);

            int assigned_shelter_id = 0;
            jsonInt(body, "assigned_shelter_id", assigned_shelter_id);

            std::string status = "awaiting_shelter";
            if (!jsonString(body, "status", status)) {
                status = (assigned_shelter_id > 0) ? "sheltered" : "awaiting_shelter";
            }

            std::string shelterSql = (assigned_shelter_id > 0) ? std::to_string(assigned_shelter_id) : "NULL";

            std::ostringstream sql;
            sql << "UPDATE victims SET "
                << "name = " << Database::quote(name) << ", "
                << "age = " << age << ", "
                << "health_status = " << Database::quote(health_status) << ", "
                << "accompanying_people = " << accompanying_people << ", "
                << "emergency_category = " << Database::quote(emergency_category) << ", "
                << "current_location_id = " << current_location_id << ", "
                << "medical_assistance = " << (medical_assistance ? 1 : 0) << ", "
                << "shelter_id = " << shelterSql << ", "
                << "assigned_shelter_id = " << shelterSql << ", "
                << "status = " << Database::quote(status)
                << " WHERE id = " << id;
            db.execScript(sql.str());

            logShelterEvent(db, "Updated victim #" + std::to_string(id) + " (" + name + ")");

            VictimRecord updated;
            db.query(
                "SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people, "
                "v.emergency_category, v.current_location_id, COALESCE(l.name, ''), v.medical_assistance, "
                "COALESCE(v.assigned_shelter_id, 0), COALESCE(s.name, ''), v.status, v.created_at "
                "FROM victims v LEFT JOIN locations l ON v.current_location_id = l.id "
                "LEFT JOIN shelters s ON v.assigned_shelter_id = s.id WHERE v.id = " + std::to_string(id),
                [&updated](sqlite3_stmt* st) {
                    updated = rowToVictim(st);
                });

            return jsonOk(victimToJson(updated));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // DELETE /api/victims/<int> - delete victim and update shelter occupancy
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/victims/<int>").methods(crow::HTTPMethod::Delete)([&db](int id) {
        try {
            long long exists = db.scalarInt("SELECT COUNT(*) FROM victims WHERE id = " + std::to_string(id));
            if (exists == 0) return jsonError(404, "Victim not found");

            int assigned_shelter = 0;
            int accompanying = 0;
            std::string name = "";

            db.query("SELECT assigned_shelter_id, accompanying_people, name FROM victims WHERE id = " + std::to_string(id), [&](sqlite3_stmt* st) {
                assigned_shelter = sqlite3_column_int(st, 0);
                accompanying = sqlite3_column_int(st, 1);
                name = colText(st, 2);
            });

            // If victim was sheltered, decrement shelter occupancy
            if (assigned_shelter > 0) {
                int people_count = 1 + accompanying;
                db.execScript(
                    "UPDATE shelters SET "
                    "current_occupancy = MAX(0, current_occupancy - " + std::to_string(people_count) + "), "
                    "occupied = MAX(0, occupied - " + std::to_string(people_count) + ") "
                    "WHERE id = " + std::to_string(assigned_shelter));

                // Recalculate status
                db.execScript(
                    "UPDATE shelters SET status = CASE "
                    "WHEN current_occupancy >= total_capacity THEN 'Full' "
                    "WHEN current_occupancy >= total_capacity * 0.9 THEN 'Limited Capacity' "
                    "ELSE 'Available' END WHERE id = " + std::to_string(assigned_shelter));
            }

            db.execScript("DELETE FROM victims WHERE id = " + std::to_string(id));
            logShelterEvent(db, "Victim #" + std::to_string(id) + " (" + name + ") deleted");

            crow::json::wvalue res;
            res["deleted"] = true;
            res["id"] = id;
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // GET /api/victims/search - search via C++ Hash Map & BST
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/victims/search")([&db](const crow::request& req) {
        try {
            std::string q = req.url_params.get("q") ? req.url_params.get("q") : "";
            if (q.empty()) return jsonError(400, "Missing search query parameter 'q'");

            // Load all victims into C++ Hash Map and BST
            ChainingHashMap<int, VictimRecord> victimMap;
            BinarySearchTree<int, VictimRecord> victimBst;

            db.query(
                "SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people, "
                "v.emergency_category, v.current_location_id, COALESCE(l.name, ''), v.medical_assistance, "
                "COALESCE(v.assigned_shelter_id, 0), COALESCE(s.name, ''), v.status, v.created_at "
                "FROM victims v LEFT JOIN locations l ON v.current_location_id = l.id "
                "LEFT JOIN shelters s ON v.assigned_shelter_id = s.id",
                [&](sqlite3_stmt* st) {
                    VictimRecord vr = rowToVictim(st);
                    victimMap.insert(vr.id, vr);
                    victimBst.insert(vr.id, vr);
                });

            std::vector<crow::json::wvalue> matches;

            // 1. Direct ID lookup check
            bool isNumber = !q.empty() && std::all_of(q.begin(), q.end(), ::isdigit);
            if (isNumber) {
                int searchId = std::stoi(q);
                auto* found = victimMap.find(searchId);
                if (found) {
                    matches.push_back(victimToJson(*found));
                }
            }

            // 2. String search across records
            std::string qLower = toLower(q);
            std::vector<VictimRecord> all = victimMap.allValues();
            for (const auto& vr : all) {
                if (isNumber && vr.id == std::stoi(q)) continue; // already added
                if (toLower(vr.name).find(qLower) != std::string::npos ||
                    toLower(vr.emergency_category).find(qLower) != std::string::npos) {
                    matches.push_back(victimToJson(vr));
                }
            }

            crow::json::wvalue res;
            res = std::move(matches);
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // GET /api/shelters - list all shelters with live occupancy
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/shelters")([&db] {
        try {
            std::string sql = 
                "SELECT s.id, s.name, s.location_id, COALESCE(l.name, ''), "
                "s.total_capacity, s.current_occupancy, s.has_medical, s.facilities, s.status "
                "FROM shelters s LEFT JOIN locations l ON s.location_id = l.id ORDER BY s.id";

            std::vector<crow::json::wvalue> list;
            db.query(sql, [&](sqlite3_stmt* st) {
                list.push_back(shelterToJson(rowToShelter(st)));
            });

            crow::json::wvalue res;
            res = std::move(list);
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // POST /api/shelters - add a new shelter
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/shelters").methods(crow::HTTPMethod::Post)([&db](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            std::string name;
            if (!jsonString(body, "name", name) || name.empty()) {
                return jsonError(400, "Missing or empty 'name'");
            }

            int location_id = 1;
            if (!jsonInt(body, "location_id", location_id) || location_id <= 0) {
                return jsonError(400, "Missing or invalid 'location_id'");
            }

            int total_capacity = 100;
            if (!jsonInt(body, "total_capacity", total_capacity) && !jsonInt(body, "capacity", total_capacity)) {
                return jsonError(400, "Missing or invalid 'total_capacity'");
            }
            if (total_capacity <= 0) return jsonError(400, "Capacity must be greater than zero");

            bool has_medical = false;
            jsonBool(body, "has_medical", has_medical);

            std::string facilities = "Basic Shelter, Food, Clean Water";
            jsonString(body, "facilities", facilities);

            std::string status = "Available";

            std::ostringstream sql;
            sql << "INSERT INTO shelters (name, location_id, capacity, total_capacity, occupied, current_occupancy, has_medical, facilities, status) VALUES ("
                << Database::quote(name) << ", "
                << location_id << ", "
                << total_capacity << ", "
                << total_capacity << ", "
                << 0 << ", "
                << 0 << ", "
                << (has_medical ? 1 : 0) << ", "
                << Database::quote(facilities) << ", "
                << Database::quote(status) << ")";
            db.execScript(sql.str());

            long long newId = db.scalarInt("SELECT last_insert_rowid()");
            logShelterEvent(db, "Opened new shelter #" + std::to_string(newId) + " (" + name + ", capacity: " + std::to_string(total_capacity) + ")");

            ShelterInfo created;
            db.query(
                "SELECT s.id, s.name, s.location_id, COALESCE(l.name, ''), "
                "s.total_capacity, s.current_occupancy, s.has_medical, s.facilities, s.status "
                "FROM shelters s LEFT JOIN locations l ON s.location_id = l.id WHERE s.id = " + std::to_string(newId),
                [&created](sqlite3_stmt* st) {
                    created = rowToShelter(st);
                });

            crow::response res(201, shelterToJson(created));
            res.set_header("Content-Type", "application/json");
            return res;
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // PUT /api/shelters/<int> - update shelter capacity & details
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/shelters/<int>").methods(crow::HTTPMethod::Put)([&db](const crow::request& req, int id) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            long long exists = db.scalarInt("SELECT COUNT(*) FROM shelters WHERE id = " + std::to_string(id));
            if (exists == 0) return jsonError(404, "Shelter not found");

            int current_occupancy = static_cast<int>(db.scalarInt("SELECT current_occupancy FROM shelters WHERE id = " + std::to_string(id)));

            std::string name;
            if (!jsonString(body, "name", name) || name.empty()) {
                return jsonError(400, "Missing or empty 'name'");
            }

            int location_id = 1;
            jsonInt(body, "location_id", location_id);

            int total_capacity = 0;
            if (!jsonInt(body, "total_capacity", total_capacity) && !jsonInt(body, "capacity", total_capacity)) {
                return jsonError(400, "Missing 'total_capacity'");
            }
            if (total_capacity < current_occupancy) {
                return jsonError(400, "New capacity (" + std::to_string(total_capacity) + 
                                      ") cannot be less than current occupancy (" + std::to_string(current_occupancy) + ")");
            }

            bool has_medical = false;
            jsonBool(body, "has_medical", has_medical);

            std::string facilities = "";
            jsonString(body, "facilities", facilities);

            std::string status = computeShelterStatus(current_occupancy, total_capacity);

            std::ostringstream sql;
            sql << "UPDATE shelters SET "
                << "name = " << Database::quote(name) << ", "
                << "location_id = " << location_id << ", "
                << "capacity = " << total_capacity << ", "
                << "total_capacity = " << total_capacity << ", "
                << "has_medical = " << (has_medical ? 1 : 0) << ", "
                << "facilities = " << Database::quote(facilities) << ", "
                << "status = " << Database::quote(status)
                << " WHERE id = " << id;
            db.execScript(sql.str());

            logShelterEvent(db, "Updated shelter #" + std::to_string(id) + " (" + name + ")");

            ShelterInfo updated;
            db.query(
                "SELECT s.id, s.name, s.location_id, COALESCE(l.name, ''), "
                "s.total_capacity, s.current_occupancy, s.has_medical, s.facilities, s.status "
                "FROM shelters s LEFT JOIN locations l ON s.location_id = l.id WHERE s.id = " + std::to_string(id),
                [&updated](sqlite3_stmt* st) {
                    updated = rowToShelter(st);
                });

            return jsonOk(shelterToJson(updated));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // DELETE /api/shelters/<int> - delete shelter only if empty
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/shelters/<int>").methods(crow::HTTPMethod::Delete)([&db](int id) {
        try {
            long long exists = db.scalarInt("SELECT COUNT(*) FROM shelters WHERE id = " + std::to_string(id));
            if (exists == 0) return jsonError(404, "Shelter not found");

            long long occupancy = db.scalarInt("SELECT current_occupancy FROM shelters WHERE id = " + std::to_string(id));
            if (occupancy > 0) {
                return jsonError(400, "Cannot delete shelter with active occupants (" + std::to_string(occupancy) + " occupants present)");
            }

            db.execScript("DELETE FROM shelters WHERE id = " + std::to_string(id));
            logShelterEvent(db, "Closed shelter #" + std::to_string(id));

            crow::json::wvalue res;
            res["deleted"] = true;
            res["id"] = id;
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // GET /api/shelters/recommend - run Dijkstra shelter recommendation
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/shelters/recommend")([&db](const crow::request& req) {
        try {
            int victim_id = req.url_params.get("victim_id") ? std::atoi(req.url_params.get("victim_id")) : 0;
            if (victim_id <= 0) return jsonError(400, "Missing or invalid 'victim_id'");

            VictimRecord victim;
            bool vFound = false;
            db.query(
                "SELECT v.id, v.name, v.age, v.gender, v.health_status, v.accompanying_people, "
                "v.emergency_category, v.current_location_id, COALESCE(l.name, ''), v.medical_assistance, "
                "COALESCE(v.assigned_shelter_id, 0), COALESCE(s.name, ''), v.status, v.created_at "
                "FROM victims v LEFT JOIN locations l ON v.current_location_id = l.id "
                "LEFT JOIN shelters s ON v.assigned_shelter_id = s.id WHERE v.id = " + std::to_string(victim_id),
                [&](sqlite3_stmt* st) {
                    victim = rowToVictim(st);
                    vFound = true;
                });

            if (!vFound) return jsonError(404, "Victim not found");

            // Load shelters
            std::vector<ShelterInfo> shelters;
            db.query(
                "SELECT s.id, s.name, s.location_id, COALESCE(l.name, ''), "
                "s.total_capacity, s.current_occupancy, s.has_medical, s.facilities, s.status "
                "FROM shelters s LEFT JOIN locations l ON s.location_id = l.id ORDER BY s.id",
                [&shelters](sqlite3_stmt* st) {
                    shelters.push_back(rowToShelter(st));
                });

            // Load Graph
            GraphData gd = loadRoadGraph(db);

            RecommendationResponse recResp = ShelterRecommender::recommendShelter(victim, shelters, gd.graph, gd.locationNames);

            crow::json::wvalue res;
            res["found"] = recResp.found;
            res["message"] = recResp.message;
            res["victim"] = victimToJson(victim);

            if (recResp.found) {
                // Primary
                crow::json::wvalue prim;
                prim["shelter"] = shelterToJson(recResp.primary_recommendation.shelter);
                prim["distance_km"] = recResp.primary_recommendation.distance_km;
                prim["reason"] = recResp.primary_recommendation.reason;
                prim["score"] = recResp.primary_recommendation.recommendation_score;

                std::vector<crow::json::wvalue> pathIds;
                for (int pid : recResp.primary_recommendation.path_node_ids) pathIds.push_back(pid);
                prim["path_node_ids"] = std::move(pathIds);

                std::vector<crow::json::wvalue> pathNames;
                for (const auto& nm : recResp.primary_recommendation.path_node_names) pathNames.push_back(nm);
                prim["path_node_names"] = std::move(pathNames);

                res["primary_recommendation"] = std::move(prim);

                // Alternatives
                std::vector<crow::json::wvalue> alts;
                for (const auto& alt : recResp.alternatives) {
                    crow::json::wvalue a;
                    a["shelter"] = shelterToJson(alt.shelter);
                    a["distance_km"] = alt.distance_km;
                    a["reason"] = alt.reason;
                    a["score"] = alt.recommendation_score;

                    std::vector<crow::json::wvalue> altPathIds;
                    for (int pid : alt.path_node_ids) altPathIds.push_back(pid);
                    a["path_node_ids"] = std::move(altPathIds);

                    std::vector<crow::json::wvalue> altPathNames;
                    for (const auto& nm : alt.path_node_names) altPathNames.push_back(nm);
                    a["path_node_names"] = std::move(altPathNames);

                    alts.push_back(std::move(a));
                }
                res["alternatives"] = std::move(alts);
            }

            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // POST /api/shelters/assign - transactionally assign victim to shelter
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/shelters/assign").methods(crow::HTTPMethod::Post)([&db](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            int victim_id = 0;
            if (!jsonInt(body, "victim_id", victim_id) || victim_id <= 0) {
                return jsonError(400, "Missing or invalid 'victim_id'");
            }

            int shelter_id = 0;
            if (!jsonInt(body, "shelter_id", shelter_id) || shelter_id <= 0) {
                return jsonError(400, "Missing or invalid 'shelter_id'");
            }

            // Fetch victim details
            long long vExists = db.scalarInt("SELECT COUNT(*) FROM victims WHERE id = " + std::to_string(victim_id));
            if (vExists == 0) return jsonError(404, "Victim not found");

            int accompanying = 0;
            int previous_shelter = 0;
            std::string victim_name = "";

            db.query("SELECT accompanying_people, COALESCE(assigned_shelter_id, 0), name FROM victims WHERE id = " + std::to_string(victim_id), [&](sqlite3_stmt* st) {
                accompanying = sqlite3_column_int(st, 0);
                previous_shelter = sqlite3_column_int(st, 1);
                victim_name = colText(st, 2);
            });

            int people_count = 1 + accompanying;

            // Check target shelter capacity
            long long sExists = db.scalarInt("SELECT COUNT(*) FROM shelters WHERE id = " + std::to_string(shelter_id));
            if (sExists == 0) return jsonError(404, "Shelter not found");

            int total_cap = 0, curr_occ = 0;
            std::string shelter_name = "";
            db.query("SELECT total_capacity, current_occupancy, name FROM shelters WHERE id = " + std::to_string(shelter_id), [&](sqlite3_stmt* st) {
                total_cap = sqlite3_column_int(st, 0);
                curr_occ = sqlite3_column_int(st, 1);
                shelter_name = colText(st, 2);
            });

            if (curr_occ + people_count > total_cap) {
                return jsonError(400, "Shelter capacity exceeded! Requested: " + std::to_string(people_count) + 
                                      " spots, Available: " + std::to_string(total_cap - curr_occ) + " spots.");
            }

            // Atomic transaction: update old shelter if any, update new shelter, update victim
            std::ostringstream tx;
            tx << "BEGIN TRANSACTION;\n";

            if (previous_shelter > 0 && previous_shelter != shelter_id) {
                tx << "UPDATE shelters SET "
                   << "current_occupancy = MAX(0, current_occupancy - " << people_count << "), "
                   << "occupied = MAX(0, occupied - " << people_count << ") "
                   << "WHERE id = " << previous_shelter << ";\n"
                   << "UPDATE shelters SET status = CASE "
                   << "WHEN current_occupancy >= total_capacity THEN 'Full' "
                   << "WHEN current_occupancy >= total_capacity * 0.9 THEN 'Limited Capacity' "
                   << "ELSE 'Available' END WHERE id = " << previous_shelter << ";\n";
            }

            if (previous_shelter != shelter_id) {
                tx << "UPDATE shelters SET "
                   << "current_occupancy = current_occupancy + " << people_count << ", "
                   << "occupied = occupied + " << people_count << " "
                   << "WHERE id = " << shelter_id << ";\n"
                   << "UPDATE shelters SET status = CASE "
                   << "WHEN current_occupancy >= total_capacity THEN 'Full' "
                   << "WHEN current_occupancy >= total_capacity * 0.9 THEN 'Limited Capacity' "
                   << "ELSE 'Available' END WHERE id = " << shelter_id << ";\n";
            }

            tx << "UPDATE victims SET "
               << "shelter_id = " << shelter_id << ", "
               << "assigned_shelter_id = " << shelter_id << ", "
               << "status = 'sheltered' "
               << "WHERE id = " << victim_id << ";\n"
               << "COMMIT;";

            db.execScript(tx.str());

            logShelterEvent(db, "Assigned victim #" + std::to_string(victim_id) + " (" + victim_name + 
                                ", " + std::to_string(people_count) + " people) to " + shelter_name);

            crow::json::wvalue res;
            res["success"] = true;
            res["message"] = "Successfully assigned to " + shelter_name;
            res["victim_id"] = victim_id;
            res["shelter_id"] = shelter_id;
            res["people_assigned"] = people_count;

            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // GET /api/shelters/overview - summary KPI cards for Module 4
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/shelters/overview")([&db] {
        try {
            crow::json::wvalue ov;
            ov["total_victims"] = db.scalarInt("SELECT COUNT(*) FROM victims");
            ov["awaiting_shelter"] = db.scalarInt("SELECT COUNT(*) FROM victims WHERE status = 'awaiting_shelter' OR assigned_shelter_id IS NULL");
            ov["sheltered_victims"] = db.scalarInt("SELECT COUNT(*) FROM victims WHERE status = 'sheltered'");
            ov["total_shelters"] = db.scalarInt("SELECT COUNT(*) FROM shelters");
            
            long long total_cap = db.scalarInt("SELECT COALESCE(SUM(total_capacity), 0) FROM shelters");
            long long total_occ = db.scalarInt("SELECT COALESCE(SUM(current_occupancy), 0) FROM shelters");
            
            ov["total_capacity"] = total_cap;
            ov["current_occupancy"] = total_occ;
            ov["available_capacity"] = std::max(0LL, total_cap - total_occ);
            ov["full_shelters"] = db.scalarInt("SELECT COUNT(*) FROM shelters WHERE current_occupancy >= total_capacity OR status = 'Full'");
            ov["available_shelters"] = db.scalarInt("SELECT COUNT(*) FROM shelters WHERE current_occupancy < total_capacity AND status != 'Full'");

            return jsonOk(std::move(ov));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });
}

} // namespace aegis
