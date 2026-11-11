#include "aegis/api.hpp"
#include "resource/resource_types.hpp"
#include "resource/resource_engine.hpp"
#include "resource/resource_sort.hpp"
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace aegis {
namespace {

std::string colText(sqlite3_stmt* st, int col) {
    const unsigned char* t = sqlite3_column_text(st, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

void logResourceEvent(Database& db, const std::string& message) {
    db.execScript("INSERT INTO simulation_events (disaster_id, category, message) VALUES "
                  "((SELECT id FROM disasters WHERE status='active' ORDER BY id DESC LIMIT 1), 'resource', " +
                  Database::quote(message) + ")");
}

ResourceItem rowToResourceItem(sqlite3_stmt* st) {
    ResourceItem item;
    item.id = sqlite3_column_int(st, 0);
    item.name = colText(st, 1);
    item.category = colText(st, 2);
    item.available_quantity = sqlite3_column_int(st, 3);
    item.unit_weight = sqlite3_column_double(st, 4);
    item.value_score = sqlite3_column_int(st, 5);
    item.required_quantity = sqlite3_column_int(st, 6);
    item.shelter_id = sqlite3_column_int(st, 7);
    item.shelter_name = colText(st, 8);
    item.ratio = (item.unit_weight > 0.0) ? (static_cast<double>(item.value_score) / item.unit_weight) : 0.0;
    return item;
}

crow::json::wvalue resourceItemToJson(const ResourceItem& item) {
    crow::json::wvalue j;
    j["id"] = item.id;
    j["name"] = item.name;
    j["category"] = item.category;
    j["available_quantity"] = item.available_quantity;
    j["quantity"] = item.available_quantity;
    j["unit_weight"] = item.unit_weight;
    j["weight_kg"] = item.unit_weight;
    j["value_score"] = item.value_score;
    j["value"] = item.value_score;
    j["required_quantity"] = item.required_quantity;
    if (item.shelter_id > 0) {
        j["shelter_id"] = item.shelter_id;
        j["shelter_name"] = item.shelter_name;
    } else {
        j["shelter_id"] = nullptr;
        j["shelter_name"] = "Central Depot";
    }
    j["ratio"] = item.ratio;
    return j;
}

} // namespace

void registerResourceRoutes(App& app, Database& db) {
    // -------------------------------------------------------------
    // GET /api/resources - list all inventory items
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/resources")([&db] {
        try {
            std::vector<ResourceItem> items;
            db.query(
                "SELECT r.id, r.name, r.category, r.available_quantity, r.unit_weight, r.value_score, "
                "r.required_quantity, COALESCE(r.shelter_id, 0), COALESCE(s.name, '') "
                "FROM resources r LEFT JOIN shelters s ON r.shelter_id = s.id ORDER BY r.id",
                [&items](sqlite3_stmt* st) {
                    items.push_back(rowToResourceItem(st));
                });

            crow::json::wvalue res;
            std::vector<crow::json::wvalue> list;
            list.reserve(items.size());
            for (const auto& item : items) {
                list.push_back(resourceItemToJson(item));
            }
            res = std::move(list);
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // POST /api/resources - add a new resource
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/resources").methods(crow::HTTPMethod::Post)([&db](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            std::string name, category;
            if (!jsonString(body, "name", name) || name.empty()) {
                return jsonError(400, "Missing or empty 'name'");
            }
            if (!jsonString(body, "category", category) || category.empty()) {
                return jsonError(400, "Missing or empty 'category'");
            }

            int available_quantity = 0;
            if (!jsonInt(body, "available_quantity", available_quantity) && !jsonInt(body, "quantity", available_quantity)) {
                return jsonError(400, "Missing or invalid 'available_quantity'");
            }
            if (available_quantity < 0) {
                return jsonError(400, "Available quantity cannot be negative");
            }

            double unit_weight = 0.0;
            if (!jsonNumber(body, "unit_weight", unit_weight) && !jsonNumber(body, "weight_kg", unit_weight)) {
                return jsonError(400, "Missing or invalid 'unit_weight'");
            }
            if (unit_weight <= 0.0) {
                return jsonError(400, "Unit weight must be greater than 0");
            }

            int value_score = 0;
            if (!jsonInt(body, "value_score", value_score) && !jsonInt(body, "value", value_score)) {
                return jsonError(400, "Missing or invalid 'value_score'");
            }
            if (value_score <= 0) {
                return jsonError(400, "Value score must be greater than 0");
            }

            int shelter_id = 0;
            jsonInt(body, "shelter_id", shelter_id);

            int required_quantity = 0;
            jsonInt(body, "required_quantity", required_quantity);

            // Check uniqueness
            long long existing = db.scalarInt("SELECT COUNT(*) FROM resources WHERE LOWER(name) = LOWER(" + Database::quote(name) + ")");
            if (existing > 0) {
                return jsonError(400, "A resource with this name already exists");
            }

            std::string shelterSql = (shelter_id > 0) ? std::to_string(shelter_id) : "NULL";

            std::ostringstream sql;
            sql << "INSERT INTO resources (name, category, available_quantity, quantity, unit_weight, weight_kg, value_score, value, required_quantity, shelter_id) VALUES ("
                << Database::quote(name) << ", "
                << Database::quote(category) << ", "
                << available_quantity << ", "
                << available_quantity << ", "
                << unit_weight << ", "
                << unit_weight << ", "
                << value_score << ", "
                << value_score << ", "
                << required_quantity << ", "
                << shelterSql << ")";
            db.execScript(sql.str());

            long long newId = db.scalarInt("SELECT last_insert_rowid()");

            logResourceEvent(db, "New emergency supply added: " + name + " (" + std::to_string(available_quantity) + " units, " + std::to_string(unit_weight) + " kg/unit)");

            crow::json::wvalue res = db.firstRow(
                "SELECT r.id, r.name, r.category, r.available_quantity, r.quantity, r.unit_weight, r.weight_kg, r.value_score, r.value, "
                "r.required_quantity, r.shelter_id, COALESCE(s.name, 'Central Depot') as shelter_name "
                "FROM resources r LEFT JOIN shelters s ON r.shelter_id = s.id WHERE r.id = " + std::to_string(newId));

            double ratio = (unit_weight > 0.0) ? (static_cast<double>(value_score) / unit_weight) : 0.0;
            res["ratio"] = ratio;

            crow::response response(201, res);
            response.set_header("Content-Type", "application/json");
            return response;
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // PUT /api/resources/<int> - update existing resource
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/resources/<int>").methods(crow::HTTPMethod::Put)([&db](const crow::request& req, int id) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            long long exists = db.scalarInt("SELECT COUNT(*) FROM resources WHERE id = " + std::to_string(id));
            if (exists == 0) return jsonError(404, "Resource not found");

            std::string name, category;
            if (!jsonString(body, "name", name) || name.empty()) {
                return jsonError(400, "Missing or empty 'name'");
            }
            if (!jsonString(body, "category", category) || category.empty()) {
                return jsonError(400, "Missing or empty 'category'");
            }

            int available_quantity = 0;
            if (!jsonInt(body, "available_quantity", available_quantity) && !jsonInt(body, "quantity", available_quantity)) {
                return jsonError(400, "Missing or invalid 'available_quantity'");
            }
            if (available_quantity < 0) {
                return jsonError(400, "Available quantity cannot be negative");
            }

            double unit_weight = 0.0;
            if (!jsonNumber(body, "unit_weight", unit_weight) && !jsonNumber(body, "weight_kg", unit_weight)) {
                return jsonError(400, "Missing or invalid 'unit_weight'");
            }
            if (unit_weight <= 0.0) {
                return jsonError(400, "Unit weight must be greater than 0");
            }

            int value_score = 0;
            if (!jsonInt(body, "value_score", value_score) && !jsonInt(body, "value", value_score)) {
                return jsonError(400, "Missing or invalid 'value_score'");
            }
            if (value_score <= 0) {
                return jsonError(400, "Value score must be greater than 0");
            }

            int shelter_id = 0;
            jsonInt(body, "shelter_id", shelter_id);

            int required_quantity = 0;
            jsonInt(body, "required_quantity", required_quantity);

            // Check name collision with another resource
            long long duplicate = db.scalarInt(
                "SELECT COUNT(*) FROM resources WHERE LOWER(name) = LOWER(" + Database::quote(name) + ") AND id != " + std::to_string(id));
            if (duplicate > 0) {
                return jsonError(400, "Another resource with this name already exists");
            }

            std::string shelterSql = (shelter_id > 0) ? std::to_string(shelter_id) : "NULL";

            std::ostringstream sql;
            sql << "UPDATE resources SET "
                << "name = " << Database::quote(name) << ", "
                << "category = " << Database::quote(category) << ", "
                << "available_quantity = " << available_quantity << ", "
                << "quantity = " << available_quantity << ", "
                << "unit_weight = " << unit_weight << ", "
                << "weight_kg = " << unit_weight << ", "
                << "value_score = " << value_score << ", "
                << "value = " << value_score << ", "
                << "required_quantity = " << required_quantity << ", "
                << "shelter_id = " << shelterSql
                << " WHERE id = " << id;
            db.execScript(sql.str());

            logResourceEvent(db, "Resource #" + std::to_string(id) + " (" + name + ") updated");

            crow::json::wvalue res = db.firstRow(
                "SELECT r.id, r.name, r.category, r.available_quantity, r.quantity, r.unit_weight, r.weight_kg, r.value_score, r.value, "
                "r.required_quantity, r.shelter_id, COALESCE(s.name, 'Central Depot') as shelter_name "
                "FROM resources r LEFT JOIN shelters s ON r.shelter_id = s.id WHERE r.id = " + std::to_string(id));

            double ratio = (unit_weight > 0.0) ? (static_cast<double>(value_score) / unit_weight) : 0.0;
            res["ratio"] = ratio;

            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // DELETE /api/resources/<int> - delete resource
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/resources/<int>").methods(crow::HTTPMethod::Delete)([&db](int id) {
        try {
            long long exists = db.scalarInt("SELECT COUNT(*) FROM resources WHERE id = " + std::to_string(id));
            if (exists == 0) return jsonError(404, "Resource not found");

            std::string name = "";
            db.query("SELECT name FROM resources WHERE id = " + std::to_string(id), [&name](sqlite3_stmt* st) {
                name = colText(st, 0);
            });

            db.execScript("DELETE FROM resources WHERE id = " + std::to_string(id));
            logResourceEvent(db, "Resource #" + std::to_string(id) + " (" + name + ") deleted from inventory");

            crow::json::wvalue res;
            res["deleted"] = true;
            res["id"] = id;
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // POST /api/resources/optimize - run DP and Greedy Knapsack optimization
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/resources/optimize").methods(crow::HTTPMethod::Post)([&db](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            double capacity_limit = 0.0;
            if (!jsonNumber(body, "capacity_limit", capacity_limit) && !jsonNumber(body, "capacity", capacity_limit)) {
                return jsonError(400, "Missing or invalid 'capacity_limit'");
            }
            if (capacity_limit <= 0.0) {
                return jsonError(400, "Capacity limit must be greater than 0 kg");
            }

            int shelter_id = 0;
            jsonInt(body, "shelter_id", shelter_id);

            // Build query based on optional filters
            std::string sql = "SELECT r.id, r.name, r.category, r.available_quantity, r.unit_weight, r.value_score, "
                              "r.required_quantity, COALESCE(r.shelter_id, 0), COALESCE(s.name, '') "
                              "FROM resources r LEFT JOIN shelters s ON r.shelter_id = s.id "
                              "WHERE r.available_quantity > 0";

            if (shelter_id > 0) {
                sql += " AND (r.shelter_id = " + std::to_string(shelter_id) + " OR r.shelter_id IS NULL)";
            }

            // Check if specific item_ids were sent
            std::vector<int> filter_ids;
            if (body.has("item_ids") && body["item_ids"].t() == crow::json::type::List) {
                for (const auto& el : body["item_ids"]) {
                    if (el.t() == crow::json::type::Number) {
                        filter_ids.push_back(static_cast<int>(el.d()));
                    }
                }
            }

            if (!filter_ids.empty()) {
                sql += " AND r.id IN (";
                for (size_t i = 0; i < filter_ids.size(); ++i) {
                    if (i > 0) sql += ",";
                    sql += std::to_string(filter_ids[i]);
                }
                sql += ")";
            }

            sql += " ORDER BY r.id";

            std::vector<ResourceItem> raw_items;
            db.query(sql, [&raw_items](sqlite3_stmt* st) {
                raw_items.push_back(rowToResourceItem(st));
            });

            std::vector<ResourceItem> items = ResourceEngine::filterValidItems(raw_items);

            ResourceEngine engine;
            ComparisonResult comp = engine.compareOptimizers(items, capacity_limit);

            crow::json::wvalue res;

            // DP Result
            {
                crow::json::wvalue dp_j;
                dp_j["total_weight"] = comp.dp_result.total_weight;
                dp_j["total_value"] = comp.dp_result.total_value;
                dp_j["remaining_capacity"] = comp.dp_result.remaining_capacity;
                dp_j["runtime_us"] = comp.dp_result.runtime_us;

                std::vector<crow::json::wvalue> sel;
                sel.reserve(comp.dp_result.selected_items.size());
                for (const auto& it : comp.dp_result.selected_items) {
                    sel.push_back(resourceItemToJson(it));
                }
                dp_j["selected_items"] = std::move(sel);

                std::vector<crow::json::wvalue> expl;
                expl.reserve(comp.dp_result.explanations.size());
                for (const auto& exp : comp.dp_result.explanations) {
                    expl.push_back(exp);
                }
                dp_j["explanations"] = std::move(expl);

                // DP Table matrix
                std::vector<crow::json::wvalue> table_rows;
                table_rows.reserve(comp.dp_result.dp_table.size());
                for (const auto& row : comp.dp_result.dp_table) {
                    std::vector<crow::json::wvalue> row_vals;
                    row_vals.reserve(row.size());
                    for (int val : row) {
                        row_vals.push_back(val);
                    }
                    table_rows.push_back(std::move(row_vals));
                }
                dp_j["dp_table"] = std::move(table_rows);

                std::vector<crow::json::wvalue> cap_headers;
                cap_headers.reserve(comp.dp_result.capacity_headers.size());
                for (int cap : comp.dp_result.capacity_headers) {
                    cap_headers.push_back(cap);
                }
                dp_j["capacity_headers"] = std::move(cap_headers);

                std::vector<crow::json::wvalue> it_headers;
                it_headers.reserve(comp.dp_result.item_headers.size());
                for (const auto& ih : comp.dp_result.item_headers) {
                    it_headers.push_back(ih);
                }
                dp_j["item_headers"] = std::move(it_headers);

                res["dp_result"] = std::move(dp_j);
            }

            // Greedy Result
            {
                crow::json::wvalue gr_j;
                gr_j["total_weight"] = comp.greedy_result.total_weight;
                gr_j["total_value"] = comp.greedy_result.total_value;
                gr_j["remaining_capacity"] = comp.greedy_result.remaining_capacity;
                gr_j["runtime_us"] = comp.greedy_result.runtime_us;

                std::vector<crow::json::wvalue> sel;
                sel.reserve(comp.greedy_result.selected_items.size());
                for (const auto& it : comp.greedy_result.selected_items) {
                    sel.push_back(resourceItemToJson(it));
                }
                gr_j["selected_items"] = std::move(sel);

                std::vector<crow::json::wvalue> order;
                order.reserve(comp.greedy_result.selection_order.size());
                for (const auto& ord : comp.greedy_result.selection_order) {
                    order.push_back(ord);
                }
                gr_j["selection_order"] = std::move(order);

                res["greedy_result"] = std::move(gr_j);
            }

            // Comparison
            {
                crow::json::wvalue c_j;
                c_j["value_difference"] = comp.value_difference;
                c_j["weight_difference"] = comp.weight_difference;
                c_j["winner"] = comp.winner;
                c_j["analysis_summary"] = comp.analysis_summary;
                c_j["capacity_limit"] = comp.capacity_limit;
                c_j["item_count"] = comp.item_count;
                res["comparison"] = std::move(c_j);
            }

            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // POST /api/resources/allocate - allocate resources and deduct inventory
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/resources/allocate").methods(crow::HTTPMethod::Post)([&db](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            int resource_id = 0;
            if (!jsonInt(body, "resource_id", resource_id) || resource_id <= 0) {
                return jsonError(400, "Missing or invalid 'resource_id'");
            }

            int quantity = 0;
            if (!jsonInt(body, "quantity", quantity) && !jsonInt(body, "allocated_quantity", quantity)) {
                return jsonError(400, "Missing or invalid 'quantity'");
            }
            if (quantity <= 0) {
                return jsonError(400, "Quantity must be greater than 0");
            }

            int shelter_id = 0;
            jsonInt(body, "shelter_id", shelter_id);

            int request_id = 0;
            jsonInt(body, "request_id", request_id);

            // Fetch current resource state
            long long exists = db.scalarInt("SELECT COUNT(*) FROM resources WHERE id = " + std::to_string(resource_id));
            if (exists == 0) return jsonError(404, "Resource not found");

            int available = 0;
            double unit_weight = 0.0;
            std::string res_name = "";

            db.query(
                "SELECT available_quantity, unit_weight, name FROM resources WHERE id = " + std::to_string(resource_id),
                [&](sqlite3_stmt* st) {
                    available = sqlite3_column_int(st, 0);
                    unit_weight = sqlite3_column_double(st, 1);
                    res_name = colText(st, 2);
                });

            std::string val_err;
            if (!ResourceEngine::validateAllocation(available, quantity, val_err)) {
                return jsonError(400, val_err);
            }

            double total_weight = unit_weight * quantity;
            std::string shelterSql = (shelter_id > 0) ? std::to_string(shelter_id) : "NULL";
            std::string requestSql = (request_id > 0) ? std::to_string(request_id) : "NULL";

            // Atomic transaction: update inventory and insert allocation record
            std::ostringstream tx;
            tx << "BEGIN TRANSACTION;\n"
               << "UPDATE resources SET "
               << "available_quantity = available_quantity - " << quantity << ", "
               << "quantity = quantity - " << quantity
               << " WHERE id = " << resource_id << ";\n"
               << "INSERT INTO resource_allocations (resource_id, shelter_id, request_id, allocated_quantity, quantity, total_weight, allocation_date, allocated_at) VALUES ("
               << resource_id << ", "
               << shelterSql << ", "
               << requestSql << ", "
               << quantity << ", "
               << quantity << ", "
               << total_weight << ", "
               << "datetime('now'), datetime('now'));\n"
               << "COMMIT;";

            db.execScript(tx.str());

            long long allocId = db.scalarInt("SELECT last_insert_rowid()");

            std::string destinationDesc = "Central Reserve";
            if (shelter_id > 0) {
                db.query("SELECT name FROM shelters WHERE id = " + std::to_string(shelter_id), [&destinationDesc](sqlite3_stmt* st) {
                    destinationDesc = "Shelter: " + colText(st, 0);
                });
            } else if (request_id > 0) {
                destinationDesc = "Emergency Request #" + std::to_string(request_id);
            }

            logResourceEvent(db, "Dispatched " + std::to_string(quantity) + " units of " + res_name + 
                                 " (" + std::to_string(total_weight) + " kg) to " + destinationDesc);

            crow::json::wvalue res;
            res["success"] = true;
            res["allocation_id"] = allocId;
            res["allocated_quantity"] = quantity;
            res["total_weight"] = total_weight;
            res["remaining_quantity"] = available - quantity;
            res["allocation"] = db.firstRow(
                "SELECT a.id, a.resource_id, r.name as resource_name, r.category, a.shelter_id, "
                "COALESCE(s.name, 'Field Dispatch') as shelter_name, a.request_id, a.allocated_quantity, "
                "a.total_weight, a.allocation_date "
                "FROM resource_allocations a JOIN resources r ON a.resource_id = r.id "
                "LEFT JOIN shelters s ON a.shelter_id = s.id WHERE a.id = " + std::to_string(allocId));

            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // GET /api/resources/allocations - list distribution records
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/resources/allocations")([&db] {
        try {
            return jsonOk(db.rows(
                "SELECT a.id, a.resource_id, r.name as resource_name, r.category, a.shelter_id, "
                "COALESCE(s.name, 'Field Dispatch') as shelter_name, a.request_id, a.allocated_quantity, "
                "a.total_weight, a.allocation_date "
                "FROM resource_allocations a JOIN resources r ON a.resource_id = r.id "
                "LEFT JOIN shelters s ON a.shelter_id = s.id ORDER BY a.id DESC"));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // GET /api/resources/overview - live summary metrics
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/resources/overview")([&db] {
        try {
            crow::json::wvalue ov;
            ov["total_items"] = db.scalarInt("SELECT COUNT(*) FROM resources");
            ov["total_units"] = db.scalarInt("SELECT COALESCE(SUM(available_quantity), 0) FROM resources");
            
            double total_weight = 0.0;
            db.query("SELECT COALESCE(SUM(available_quantity * unit_weight), 0.0) FROM resources", [&total_weight](sqlite3_stmt* st) {
                total_weight = sqlite3_column_double(st, 0);
            });
            ov["total_weight_kg"] = total_weight;

            ov["total_priority_value"] = db.scalarInt("SELECT COALESCE(SUM(available_quantity * value_score), 0) FROM resources");
            ov["total_allocations"] = db.scalarInt("SELECT COUNT(*) FROM resource_allocations");

            ov["category_counts"] = db.rows(
                "SELECT category, COUNT(*) as count, SUM(available_quantity) as total_units, "
                "ROUND(SUM(available_quantity * unit_weight), 1) as category_weight "
                "FROM resources GROUP BY category ORDER BY count DESC");

            return jsonOk(std::move(ov));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });
}

} // namespace aegis
