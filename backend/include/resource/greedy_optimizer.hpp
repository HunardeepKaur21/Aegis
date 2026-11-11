#pragma once
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "resource/resource_types.hpp"
#include "resource/resource_sort.hpp"

namespace aegis {

class GreedyOptimizer {
public:
    static GreedyResult solve(const std::vector<ResourceItem>& items, double capacity_kg) {
        auto start_time = std::chrono::high_resolution_clock::now();
        GreedyResult result;

        if (items.empty() || capacity_kg <= 0.0) {
            auto end_time = std::chrono::high_resolution_clock::now();
            result.runtime_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();
            result.remaining_capacity = std::max(0.0, capacity_kg);
            return result;
        }

        // Prepare items with calculated ratio
        std::vector<ResourceItem> candidates = items;
        for (auto& item : candidates) {
            item.ratio = (item.unit_weight > 0.0) ? (static_cast<double>(item.value_score) / item.unit_weight) : 0.0;
        }

        // Sort descending by ratio using our custom merge sort
        sortResources(candidates, ResourceSortField::Ratio, false);

        double current_weight = 0.0;
        int current_value = 0;
        std::vector<ResourceItem> chosen;
        std::vector<std::string> selection_order;

        for (const auto& item : candidates) {
            if (current_weight + item.unit_weight <= capacity_kg + 1e-5) {
                chosen.push_back(item);
                current_weight += item.unit_weight;
                current_value += item.value_score;

                std::ostringstream ss;
                ss << "Picked '" << item.name << "' (ratio: " 
                   << std::fixed << std::setprecision(2) << item.ratio 
                   << ", val: " << item.value_score 
                   << ", wt: " << std::fixed << std::setprecision(1) << item.unit_weight << " kg)";
                selection_order.push_back(ss.str());
            } else {
                std::ostringstream ss;
                ss << "Skipped '" << item.name << "' (wt: " 
                   << std::fixed << std::setprecision(1) << item.unit_weight 
                   << " kg exceeds remaining " 
                   << std::fixed << std::setprecision(1) << (capacity_kg - current_weight) << " kg)";
                selection_order.push_back(ss.str());
            }
        }

        result.selected_items = chosen;
        result.total_weight = current_weight;
        result.total_value = current_value;
        result.remaining_capacity = std::max(0.0, capacity_kg - current_weight);
        result.selection_order = selection_order;

        auto end_time = std::chrono::high_resolution_clock::now();
        result.runtime_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();

        return result;
    }
};

} // namespace aegis
