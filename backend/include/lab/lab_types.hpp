#pragma once
#include <string>
#include <vector>

namespace aegis {

struct LabStep {
    int step_number = 0;
    std::string description;
    std::vector<int> array_state;
    std::vector<std::string> string_array_state;
    int primary_index = -1;
    int secondary_index = -1;
    bool is_swap = false;
    bool is_comparison = false;
    std::vector<int> highlighted_indices;
};

struct LabRunResult {
    std::string category;
    std::string algorithm;
    std::string operation;
    bool success = true;
    std::string message;
    double runtime_us = 0.0;
    long long comparisons = 0;
    long long swaps = 0;
    std::string time_complexity;
    std::string space_complexity;

    // Numerical or String result
    std::string output_value;
    std::vector<int> result_array;
    std::vector<std::string> result_strings;

    // Step-by-step animation trace
    std::vector<LabStep> steps;

    // Structured data for Tree, Graph, Hashing, DP
    std::vector<std::string> traversal_order;
    std::vector<std::vector<int>> matrix_data;
    std::vector<std::string> row_headers;
    std::vector<std::string> col_headers;
    std::vector<std::pair<std::string, std::string>> key_value_pairs;
};

struct LabCompareResult {
    std::string algorithm_a;
    std::string algorithm_b;
    LabRunResult result_a;
    LabRunResult result_b;
    std::string summary;
    std::string winner;
};

struct AlgorithmMeta {
    std::string id;
    std::string name;
    std::string category;
    std::string time_complexity;
    std::string space_complexity;
    std::string description;
    std::vector<std::string> supported_operations;
};

} // namespace aegis
