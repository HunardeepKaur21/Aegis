#include "resource/resource_engine.hpp"
#include <iomanip>
#include <sstream>

namespace aegis {

ComparisonResult ResourceEngine::compareOptimizers(const std::vector<ResourceItem>& items, double capacity_kg) {
    ComparisonResult comp;
    comp.capacity_limit = capacity_kg;
    comp.item_count = static_cast<int>(items.size());

    comp.dp_result = KnapsackOptimizer::solve(items, capacity_kg);
    comp.greedy_result = GreedyOptimizer::solve(items, capacity_kg);

    comp.value_difference = comp.dp_result.total_value - comp.greedy_result.total_value;
    comp.weight_difference = comp.dp_result.total_weight - comp.greedy_result.total_weight;

    std::ostringstream summary;
    if (comp.value_difference > 0) {
        comp.winner = "Dynamic Programming (Optimal)";
        double gain_pct = comp.greedy_result.total_value > 0 
            ? (static_cast<double>(comp.value_difference) / comp.greedy_result.total_value * 100.0) 
            : 100.0;
        summary << "DP achieved " << comp.dp_result.total_value << " total priority vs Greedy's " 
                << comp.greedy_result.total_value << " (" << std::fixed << std::setprecision(1) 
                << "+" << gain_pct << "% improvement). Greedy made locally optimal ratio choices that trapped it into suboptimal capacity utilization.";
    } else if (comp.value_difference == 0) {
        if (comp.dp_result.total_weight < comp.greedy_result.total_weight) {
            comp.winner = "Dynamic Programming (Lower Weight)";
            summary << "Both algorithms reached equal priority (" << comp.dp_result.total_value 
                    << "), but DP achieved this using " 
                    << std::fixed << std::setprecision(1) << (comp.greedy_result.total_weight - comp.dp_result.total_weight) 
                    << " kg less payload.";
        } else if (comp.dp_result.total_weight == comp.greedy_result.total_weight) {
            comp.winner = "Tie (Identical Solutions)";
            summary << "Both DP and Greedy found identical item sets achieving " 
                    << comp.dp_result.total_value << " priority points under " 
                    << std::fixed << std::setprecision(1) << comp.dp_result.total_weight << " kg payload.";
        } else {
            comp.winner = "Greedy (Equal Value, Lighter)";
            summary << "Both reached " << comp.dp_result.total_value << " priority, with Greedy using " 
                    << std::fixed << std::setprecision(1) << (comp.dp_result.total_weight - comp.greedy_result.total_weight) 
                    << " kg less capacity.";
        }
    } else {
        comp.winner = "Greedy";
        summary << "Greedy outperformed DP due to discrete weight scaling resolution.";
    }

    comp.analysis_summary = summary.str();
    return comp;
}

std::vector<ResourceItem> ResourceEngine::filterValidItems(const std::vector<ResourceItem>& items) {
    std::vector<ResourceItem> valid;
    for (const auto& it : items) {
        if (it.available_quantity > 0 && it.unit_weight > 0.0 && it.value_score > 0) {
            ResourceItem item_copy = it;
            item_copy.ratio = static_cast<double>(it.value_score) / it.unit_weight;
            valid.push_back(item_copy);
        }
    }
    return valid;
}

bool ResourceEngine::validateAllocation(int available_qty, int request_qty, std::string& err_msg) {
    if (request_qty <= 0) {
        err_msg = "Allocated quantity must be greater than zero.";
        return false;
    }
    if (request_qty > available_qty) {
        err_msg = "Requested quantity (" + std::to_string(request_qty) + 
                  ") exceeds available inventory (" + std::to_string(available_qty) + ").";
        return false;
    }
    return true;
}

} // namespace aegis
