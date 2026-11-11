#pragma once
#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include "lab/lab_types.hpp"

namespace aegis {

class LabSearching {
public:
    static LabRunResult run(const std::string& algo_name, const std::vector<int>& input_data, int target) {
        std::string algo = algo_name;
        std::transform(algo.begin(), algo.end(), algo.begin(), ::tolower);

        if (algo == "binary" || algo == "binary search") return binarySearch(input_data, target);
        if (algo == "recursive binary" || algo == "recursive binary search") return recursiveBinarySearch(input_data, target);
        return linearSearch(input_data, target);
    }

private:
    static void addStep(LabRunResult& res, const std::vector<int>& arr, const std::string& desc,
                        int pri = -1, int sec = -1, const std::vector<int>& extra = {}) {
        if (res.steps.size() >= 100) return;
        LabStep step;
        step.step_number = static_cast<int>(res.steps.size() + 1);
        step.description = desc;
        step.array_state = arr;
        step.primary_index = pri;
        step.secondary_index = sec;
        if (pri >= 0) step.highlighted_indices.push_back(pri);
        if (sec >= 0 && sec != pri) step.highlighted_indices.push_back(sec);
        for (int x : extra) {
            if (std::find(step.highlighted_indices.begin(), step.highlighted_indices.end(), x) == step.highlighted_indices.end()) {
                step.highlighted_indices.push_back(x);
            }
        }
        res.steps.push_back(step);
    }

    static LabRunResult linearSearch(const std::vector<int>& arr, int target) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Searching";
        res.algorithm = "Linear Search";
        res.time_complexity = "O(n)";
        res.space_complexity = "O(1)";
        res.result_array = arr;

        addStep(res, arr, "Beginning linear scan for target: " + std::to_string(target), -1, -1);
        int found_idx = -1;

        for (int i = 0; i < static_cast<int>(arr.size()); ++i) {
            res.comparisons++;
            addStep(res, arr, "Examining index " + std::to_string(i) + " (value: " + std::to_string(arr[i]) + ")", i, -1);
            if (arr[i] == target) {
                found_idx = i;
                addStep(res, arr, "Target " + std::to_string(target) + " FOUND at index " + std::to_string(i) + "!", i, -1);
                break;
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        if (found_idx >= 0) {
            res.success = true;
            res.output_value = "Found at index " + std::to_string(found_idx);
            res.message = "Target " + std::to_string(target) + " found at index " + std::to_string(found_idx);
        } else {
            res.success = false;
            res.output_value = "Not Found";
            res.message = "Target " + std::to_string(target) + " not present in array";
            addStep(res, arr, "Target not found after inspecting all elements", -1, -1);
        }
        return res;
    }

    static LabRunResult binarySearch(const std::vector<int>& input_data, int target) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Searching";
        res.algorithm = "Binary Search";
        res.time_complexity = "O(log n)";
        res.space_complexity = "O(1)";

        // Binary Search operates on sorted array
        std::vector<int> arr = input_data;
        if (!std::is_sorted(arr.begin(), arr.end())) {
            std::sort(arr.begin(), arr.end());
            addStep(res, arr, "Array sorted prior to binary search", -1, -1);
        }
        res.result_array = arr;

        int low = 0;
        int high = static_cast<int>(arr.size()) - 1;
        int found_idx = -1;

        while (low <= high) {
            int mid = low + (high - low) / 2;
            res.comparisons++;
            addStep(res, arr, "Subarray [" + std::to_string(low) + "..." + std::to_string(high) + 
                              "], checking midpoint " + std::to_string(mid) + " (val: " + std::to_string(arr[mid]) + ")", 
                    mid, -1, {low, high});

            if (arr[mid] == target) {
                found_idx = mid;
                addStep(res, arr, "Target " + std::to_string(target) + " MATCHED at midpoint index " + std::to_string(mid) + "!", mid, -1);
                break;
            } else if (arr[mid] < target) {
                low = mid + 1;
                addStep(res, arr, "Target > " + std::to_string(arr[mid]) + ", discarding left half. New low: " + std::to_string(low), low, -1);
            } else {
                high = mid - 1;
                addStep(res, arr, "Target < " + std::to_string(arr[mid]) + ", discarding right half. New high: " + std::to_string(high), high, -1);
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        if (found_idx >= 0) {
            res.success = true;
            res.output_value = "Found at index " + std::to_string(found_idx);
            res.message = "Target " + std::to_string(target) + " found at index " + std::to_string(found_idx);
        } else {
            res.success = false;
            res.output_value = "Not Found";
            res.message = "Target " + std::to_string(target) + " not present in array";
            addStep(res, arr, "Search interval exhausted (low > high). Target not found", -1, -1);
        }
        return res;
    }

    static LabRunResult recursiveBinarySearch(const std::vector<int>& input_data, int target) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Searching";
        res.algorithm = "Recursive Binary Search";
        res.time_complexity = "O(log n)";
        res.space_complexity = "O(log n)";

        std::vector<int> arr = input_data;
        if (!std::is_sorted(arr.begin(), arr.end())) {
            std::sort(arr.begin(), arr.end());
            addStep(res, arr, "Array sorted prior to binary search", -1, -1);
        }
        res.result_array = arr;

        int found_idx = -1;

        std::function<void(int, int, int)> rec = [&](int low, int high, int depth) {
            if (low > high) return;
            int mid = low + (high - low) / 2;
            res.comparisons++;
            addStep(res, arr, "Recursion depth " + std::to_string(depth) + ": checking mid index " + 
                              std::to_string(mid) + " (val: " + std::to_string(arr[mid]) + ") in [" + 
                              std::to_string(low) + "..." + std::to_string(high) + "]", mid, -1, {low, high});

            if (arr[mid] == target) {
                found_idx = mid;
                addStep(res, arr, "Target found at index " + std::to_string(mid), mid, -1);
                return;
            } else if (arr[mid] < target) {
                rec(mid + 1, high, depth + 1);
            } else {
                rec(low, mid - 1, depth + 1);
            }
        };

        if (!arr.empty()) {
            rec(0, static_cast<int>(arr.size() - 1), 1);
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        if (found_idx >= 0) {
            res.success = true;
            res.output_value = "Found at index " + std::to_string(found_idx);
            res.message = "Target " + std::to_string(target) + " found at index " + std::to_string(found_idx);
        } else {
            res.success = false;
            res.output_value = "Not Found";
            res.message = "Target " + std::to_string(target) + " not present in array";
        }
        return res;
    }
};

} // namespace aegis
