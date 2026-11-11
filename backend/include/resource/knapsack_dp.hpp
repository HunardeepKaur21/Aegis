#pragma once
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "resource/resource_types.hpp"

namespace aegis {

class KnapsackOptimizer {
public:
    static constexpr int SCALE_FACTOR = 10; // Discretize 0.1 kg increments

    static KnapsackResult solve(const std::vector<ResourceItem>& items, double capacity_kg) {
        auto start_time = std::chrono::high_resolution_clock::now();
        KnapsackResult result;

        if (items.empty() || capacity_kg <= 0.0) {
            auto end_time = std::chrono::high_resolution_clock::now();
            result.runtime_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();
            result.remaining_capacity = std::max(0.0, capacity_kg);
            return result;
        }

        int n = static_cast<int>(items.size());
        int W = static_cast<int>(std::round(capacity_kg * SCALE_FACTOR));
        if (W <= 0) {
            auto end_time = std::chrono::high_resolution_clock::now();
            result.runtime_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();
            result.remaining_capacity = capacity_kg;
            return result;
        }

        // Discrete weights
        std::vector<int> weights(n);
        std::vector<int> values(n);
        for (int i = 0; i < n; ++i) {
            weights[i] = std::max(1, static_cast<int>(std::round(items[i].unit_weight * SCALE_FACTOR)));
            values[i] = items[i].value_score;
        }

        // 2D DP Table: dp[i][w] = max value using a subset of first i items with capacity w
        std::vector<std::vector<int>> dp(n + 1, std::vector<int>(W + 1, 0));

        for (int i = 1; i <= n; ++i) {
            int w_i = weights[i - 1];
            int v_i = values[i - 1];
            for (int w = 0; w <= W; ++w) {
                if (w_i <= w) {
                    dp[i][w] = std::max(dp[i - 1][w], dp[i - 1][w - w_i] + v_i);
                } else {
                    dp[i][w] = dp[i - 1][w];
                }
            }
        }

        // Backtrack to find selected items
        int curr_w = W;
        std::vector<ResourceItem> chosen;
        std::vector<std::string> explanations;

        for (int i = n; i >= 1; --i) {
            int w_i = weights[i - 1];
            int v_i = values[i - 1];
            if (curr_w >= w_i && dp[i][curr_w] == dp[i - 1][curr_w - w_i] + v_i && dp[i][curr_w] != dp[i - 1][curr_w]) {
                chosen.push_back(items[i - 1]);
                std::ostringstream ss;
                ss << "Item #" << items[i - 1].id << " '" << items[i - 1].name 
                   << "' selected: gained +" << v_i << " priority (weight: " 
                   << std::fixed << std::setprecision(1) << items[i - 1].unit_weight 
                   << " kg, state transition dp[" << i << "][" << curr_w << "] -> dp[" 
                   << (i - 1) << "][" << (curr_w - w_i) << "]).";
                explanations.push_back(ss.str());
                curr_w -= w_i;
            } else {
                std::ostringstream ss;
                ss << "Item #" << items[i - 1].id << " '" << items[i - 1].name 
                   << "' excluded: preserving previous optimal state (" << dp[i - 1][curr_w] << " pts).";
                explanations.push_back(ss.str());
            }
        }

        std::reverse(chosen.begin(), chosen.end());
        std::reverse(explanations.begin(), explanations.end());

        result.selected_items = chosen;
        result.total_value = dp[n][W];
        double total_wt = 0.0;
        for (const auto& it : chosen) {
            total_wt += it.unit_weight;
        }
        result.total_weight = total_wt;
        result.remaining_capacity = std::max(0.0, capacity_kg - total_wt);
        result.explanations = explanations;

        // DP table sampled for UI visualization
        // Select up to 15 columns: 0, intervals, and W
        std::vector<int> sampled_cols;
        int max_cols = 12;
        if (W <= max_cols) {
            for (int w = 0; w <= W; ++w) sampled_cols.push_back(w);
        } else {
            sampled_cols.push_back(0);
            double step = static_cast<double>(W) / (max_cols - 1);
            for (int k = 1; k < max_cols - 1; ++k) {
                int col = static_cast<int>(std::round(k * step));
                if (col > sampled_cols.back() && col < W) {
                    sampled_cols.push_back(col);
                }
            }
            if (sampled_cols.back() != W) {
                sampled_cols.push_back(W);
            }
        }

        result.capacity_headers.clear();
        for (int c : sampled_cols) {
            // capacity in kg rounded to 1 decimal place (stored scaled or as int kg)
            result.capacity_headers.push_back(c * 10 / SCALE_FACTOR); // gives 100g or 1kg units
        }

        result.item_headers.push_back("Base (0)");
        for (int i = 0; i < n; ++i) {
            result.item_headers.push_back(items[i].name);
        }

        result.dp_table.resize(n + 1);
        for (int i = 0; i <= n; ++i) {
            result.dp_table[i].reserve(sampled_cols.size());
            for (int c : sampled_cols) {
                result.dp_table[i].push_back(dp[i][c]);
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        result.runtime_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();

        return result;
    }
};

} // namespace aegis
