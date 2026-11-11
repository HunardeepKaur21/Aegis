#include "aegis/api.hpp"
#include "lab/lab_engine.hpp"

namespace aegis {

void registerLabRoutes(App& app, Database& db) {
    // -------------------------------------------------------------
    // GET /api/lab/algorithms - catalog of all supported algorithms
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/lab/algorithms")([] {
        try {
            auto catalog = LabEngine::getAlgorithmCatalog();
            std::vector<crow::json::wvalue> list;
            list.reserve(catalog.size());

            for (const auto& meta : catalog) {
                crow::json::wvalue item;
                item["id"] = meta.id;
                item["name"] = meta.name;
                item["category"] = meta.category;
                item["time_complexity"] = meta.time_complexity;
                item["space_complexity"] = meta.space_complexity;
                item["description"] = meta.description;

                std::vector<crow::json::wvalue> ops;
                for (const auto& op : meta.supported_operations) ops.push_back(op);
                item["supported_operations"] = std::move(ops);

                list.push_back(std::move(item));
            }

            crow::json::wvalue res;
            res = std::move(list);
            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // POST /api/lab/run - execute an algorithm with inputs
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/lab/run").methods(crow::HTTPMethod::Post)([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            std::string category = "Sorting";
            jsonString(body, "category", category);

            std::string algorithm = "Bubble Sort";
            jsonString(body, "algorithm", algorithm);

            std::string operation = "";
            jsonString(body, "operation", operation);

            int param_val = 0;
            jsonInt(body, "param_val", param_val);

            std::string str_param1 = "";
            jsonString(body, "str_param1", str_param1);

            std::string str_param2 = "";
            jsonString(body, "str_param2", str_param2);

            std::vector<int> input_data;
            if (body.has("input_data") && body["input_data"].t() == crow::json::type::List) {
                for (const auto& el : body["input_data"]) {
                    if (el.t() == crow::json::type::Number) {
                        input_data.push_back(static_cast<int>(el.d()));
                    }
                }
            }

            LabRunResult r = LabEngine::execute(category, algorithm, input_data, operation, param_val, str_param1, str_param2);

            crow::json::wvalue res;
            res["category"] = r.category;
            res["algorithm"] = r.algorithm;
            res["operation"] = r.operation;
            res["success"] = r.success;
            res["message"] = r.message;
            res["runtime_us"] = r.runtime_us;
            res["comparisons"] = r.comparisons;
            res["swaps"] = r.swaps;
            res["time_complexity"] = r.time_complexity;
            res["space_complexity"] = r.space_complexity;
            res["output_value"] = r.output_value;

            // Result array
            std::vector<crow::json::wvalue> arr_j;
            for (int x : r.result_array) arr_j.push_back(x);
            res["result_array"] = std::move(arr_j);

            // Steps for animation
            std::vector<crow::json::wvalue> steps_j;
            for (const auto& s : r.steps) {
                crow::json::wvalue sj;
                sj["step_number"] = s.step_number;
                sj["description"] = s.description;
                sj["primary_index"] = s.primary_index;
                sj["secondary_index"] = s.secondary_index;
                sj["is_swap"] = s.is_swap;
                sj["is_comparison"] = s.is_comparison;

                std::vector<crow::json::wvalue> st_arr;
                for (int x : s.array_state) st_arr.push_back(x);
                sj["array_state"] = std::move(st_arr);

                std::vector<crow::json::wvalue> hl;
                for (int h : s.highlighted_indices) hl.push_back(h);
                sj["highlighted_indices"] = std::move(hl);

                steps_j.push_back(std::move(sj));
            }
            res["steps"] = std::move(steps_j);

            // Matrix data (for DP)
            if (!r.matrix_data.empty()) {
                std::vector<crow::json::wvalue> mat_j;
                for (const auto& row : r.matrix_data) {
                    std::vector<crow::json::wvalue> row_j;
                    for (int val : row) row_j.push_back(val);
                    mat_j.push_back(std::move(row_j));
                }
                res["matrix_data"] = std::move(mat_j);

                std::vector<crow::json::wvalue> rh;
                for (const auto& h : r.row_headers) rh.push_back(h);
                res["row_headers"] = std::move(rh);

                std::vector<crow::json::wvalue> ch;
                for (const auto& h : r.col_headers) ch.push_back(h);
                res["col_headers"] = std::move(ch);
            }

            // Key-Value pairs (for Hashing and Freq)
            if (!r.key_value_pairs.empty()) {
                std::vector<crow::json::wvalue> kv_j;
                for (const auto& [k, v] : r.key_value_pairs) {
                    crow::json::wvalue pair_j;
                    pair_j["key"] = k;
                    pair_j["value"] = v;
                    kv_j.push_back(std::move(pair_j));
                }
                res["key_value_pairs"] = std::move(kv_j);
            }

            // Traversal order (for Trees & Graphs)
            if (!r.traversal_order.empty()) {
                std::vector<crow::json::wvalue> trav;
                for (const auto& t : r.traversal_order) trav.push_back(t);
                res["traversal_order"] = std::move(trav);
            }

            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });

    // -------------------------------------------------------------
    // POST /api/lab/compare - compare two algorithms concurrently
    // -------------------------------------------------------------
    CROW_ROUTE(app, "/api/lab/compare").methods(crow::HTTPMethod::Post)([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) return jsonError(400, "Invalid JSON body");

            std::string algo_a = "Quick Sort";
            jsonString(body, "algorithm_a", algo_a);

            std::string algo_b = "Bubble Sort";
            jsonString(body, "algorithm_b", algo_b);

            std::vector<int> input_data;
            if (body.has("input_data") && body["input_data"].t() == crow::json::type::List) {
                for (const auto& el : body["input_data"]) {
                    if (el.t() == crow::json::type::Number) {
                        input_data.push_back(static_cast<int>(el.d()));
                    }
                }
            }
            if (input_data.empty()) {
                input_data = {45, 12, 85, 32, 89, 39, 69, 44, 42, 1, 45, 90};
            }

            LabCompareResult comp = LabEngine::compare(algo_a, algo_b, input_data);

            crow::json::wvalue res;
            res["algorithm_a"] = comp.algorithm_a;
            res["algorithm_b"] = comp.algorithm_b;
            res["winner"] = comp.winner;
            res["summary"] = comp.summary;

            // Result A summary
            {
                crow::json::wvalue a_j;
                a_j["runtime_us"] = comp.result_a.runtime_us;
                a_j["comparisons"] = comp.result_a.comparisons;
                a_j["swaps"] = comp.result_a.swaps;
                a_j["time_complexity"] = comp.result_a.time_complexity;
                a_j["space_complexity"] = comp.result_a.space_complexity;
                res["result_a"] = std::move(a_j);
            }

            // Result B summary
            {
                crow::json::wvalue b_j;
                b_j["runtime_us"] = comp.result_b.runtime_us;
                b_j["comparisons"] = comp.result_b.comparisons;
                b_j["swaps"] = comp.result_b.swaps;
                b_j["time_complexity"] = comp.result_b.time_complexity;
                b_j["space_complexity"] = comp.result_b.space_complexity;
                res["result_b"] = std::move(b_j);
            }

            return jsonOk(std::move(res));
        } catch (const std::exception& e) {
            return jsonError(500, e.what());
        }
    });
}

} // namespace aegis
