#pragma once
#include <string>
#include <vector>
#include "resource/resource_types.hpp"
#include "resource/knapsack_dp.hpp"
#include "resource/greedy_optimizer.hpp"
#include "resource/resource_sort.hpp"

namespace aegis {

class ResourceEngine {
public:
    ResourceEngine() = default;

    // Run both DP and Greedy and produce a comprehensive comparison
    ComparisonResult compareOptimizers(const std::vector<ResourceItem>& items, double capacity_kg);

    // Filter items according to available quantity and criteria
    static std::vector<ResourceItem> filterValidItems(const std::vector<ResourceItem>& items);

    // Validate allocation input
    static bool validateAllocation(int available_qty, int request_qty, std::string& err_msg);
};

} // namespace aegis
