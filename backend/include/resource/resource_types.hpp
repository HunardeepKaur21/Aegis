#pragma once
#include <string>
#include <vector>

namespace aegis {

struct ResourceItem {
    int id = 0;
    std::string name;
    std::string category;
    int available_quantity = 0;
    double unit_weight = 0.0;
    int value_score = 0;
    int required_quantity = 0;
    int shelter_id = 0;
    std::string shelter_name;
    double ratio = 0.0; // value_score / unit_weight
};

struct KnapsackResult {
    std::vector<ResourceItem> selected_items;
    double total_weight = 0.0;
    int total_value = 0;
    double remaining_capacity = 0.0;
    double runtime_us = 0.0;
    // For DP visualization
    std::vector<std::vector<int>> dp_table;
    std::vector<int> capacity_headers;
    std::vector<std::string> item_headers;
    std::vector<std::string> explanations;
};

struct GreedyResult {
    std::vector<ResourceItem> selected_items;
    double total_weight = 0.0;
    int total_value = 0;
    double remaining_capacity = 0.0;
    double runtime_us = 0.0;
    std::vector<std::string> selection_order;
};

struct ComparisonResult {
    KnapsackResult dp_result;
    GreedyResult greedy_result;
    int value_difference = 0;      // dp - greedy
    double weight_difference = 0.0;
    std::string winner;           // "DP (Optimal)", "Greedy (Equal)", etc.
    std::string analysis_summary;
    double capacity_limit = 0.0;
    int item_count = 0;
};

struct ResourceAllocation {
    int id = 0;
    int resource_id = 0;
    std::string resource_name;
    std::string category;
    int shelter_id = 0;
    std::string shelter_name;
    int allocated_quantity = 0;
    double total_weight = 0.0;
    std::string allocation_date;
};

} // namespace aegis
