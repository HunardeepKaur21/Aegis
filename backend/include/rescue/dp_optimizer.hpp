#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>
#include "rescue/rescue_types.hpp"

namespace aegis {

struct AllocationResult {
    std::vector<EmergencyRequest> selected_requests;
    int total_capacity = 0;
    int used_capacity = 0;
    int remaining_capacity = 0;
    double total_priority_score = 0.0;
    int total_people_saved = 0;
    double runtime_us = 0.0;
};

// Dynamic Programming solver for optimal emergency rescue capacity allocation (0/1 Knapsack formulation)
inline AllocationResult optimizeRescueAllocation(
    const std::vector<EmergencyRequest>& candidates,
    int totalCapacity)
{
    auto t0 = std::chrono::steady_clock::now();
    AllocationResult res;
    res.total_capacity = std::max(0, totalCapacity);

    if (candidates.empty() || totalCapacity <= 0) {
        auto t1 = std::chrono::steady_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.remaining_capacity = res.total_capacity;
        return res;
    }

    const size_t n = candidates.size();
    const int W = totalCapacity;

    // dp[i][w] = max priority score using subset of first i requests with capacity <= w
    std::vector<std::vector<double>> dp(n + 1, std::vector<double>(W + 1, 0.0));

    for (size_t i = 1; i <= n; ++i) {
        int weight = std::max(1, candidates[i - 1].affected_people);
        double val = std::max(0.0, candidates[i - 1].priority_score);

        for (int w = 0; w <= W; ++w) {
            dp[i][w] = dp[i - 1][w];
            if (w >= weight) {
                double withCurr = dp[i - 1][w - weight] + val;
                if (withCurr > dp[i][w]) {
                    dp[i][w] = withCurr;
                }
            }
        }
    }

    // Backtrack to identify selected requests
    int currW = W;
    for (size_t i = n; i >= 1; --i) {
        int weight = std::max(1, candidates[i - 1].affected_people);
        if (currW >= weight && dp[i][currW] != dp[i - 1][currW]) {
            res.selected_requests.push_back(candidates[i - 1]);
            res.used_capacity += weight;
            res.total_people_saved += candidates[i - 1].affected_people;
            res.total_priority_score += candidates[i - 1].priority_score;
            currW -= weight;
        }
    }

    // Reverse to retain natural order
    std::reverse(res.selected_requests.begin(), res.selected_requests.end());
    res.remaining_capacity = res.total_capacity - res.used_capacity;

    auto t1 = std::chrono::steady_clock::now();
    res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    return res;
}

} // namespace aegis
