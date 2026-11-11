#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <vector>
#include "resource/resource_types.hpp"

namespace aegis {

enum class ResourceSortField {
    Ratio,      // value_score / unit_weight descending
    Value,      // value_score descending
    Weight,     // unit_weight ascending
    Quantity,   // available_quantity descending
    Name        // name alphabetical
};

// Generic stable merge sort for resource items
inline void mergeSortResources(
    std::vector<ResourceItem>& arr,
    size_t left,
    size_t right,
    const std::function<bool(const ResourceItem&, const ResourceItem&)>& comp)
{
    if (left >= right) return;
    size_t mid = left + (right - left) / 2;
    mergeSortResources(arr, left, mid, comp);
    mergeSortResources(arr, mid + 1, right, comp);

    std::vector<ResourceItem> temp;
    temp.reserve(right - left + 1);
    size_t i = left, j = mid + 1;

    while (i <= mid && j <= right) {
        if (comp(arr[i], arr[j])) {
            temp.push_back(std::move(arr[i++]));
        } else {
            temp.push_back(std::move(arr[j++]));
        }
    }
    while (i <= mid) temp.push_back(std::move(arr[i++]));
    while (j <= right) temp.push_back(std::move(arr[j++]));

    for (size_t k = 0; k < temp.size(); ++k) {
        arr[left + k] = std::move(temp[k]);
    }
}

inline void sortResources(
    std::vector<ResourceItem>& arr,
    ResourceSortField field = ResourceSortField::Ratio,
    bool ascending = false)
{
    if (arr.size() <= 1) return;

    auto comp = [field, ascending](const ResourceItem& a, const ResourceItem& b) -> bool {
        switch (field) {
            case ResourceSortField::Ratio: {
                if (std::fabs(a.ratio - b.ratio) > 1e-4) {
                    return ascending ? (a.ratio < b.ratio) : (a.ratio > b.ratio);
                }
                // Tie breaker: higher value score, then lower weight
                if (a.value_score != b.value_score) {
                    return a.value_score > b.value_score;
                }
                return a.unit_weight < b.unit_weight;
            }
            case ResourceSortField::Value: {
                if (a.value_score != b.value_score) {
                    return ascending ? (a.value_score < b.value_score) : (a.value_score > b.value_score);
                }
                return a.ratio > b.ratio;
            }
            case ResourceSortField::Weight: {
                if (std::fabs(a.unit_weight - b.unit_weight) > 1e-4) {
                    return ascending ? (a.unit_weight < b.unit_weight) : (a.unit_weight > b.unit_weight);
                }
                return a.value_score > b.value_score;
            }
            case ResourceSortField::Quantity: {
                if (a.available_quantity != b.available_quantity) {
                    return ascending ? (a.available_quantity < b.available_quantity) : (a.available_quantity > b.available_quantity);
                }
                return a.value_score > b.value_score;
            }
            case ResourceSortField::Name:
            default: {
                return ascending ? (a.name < b.name) : (a.name > b.name);
            }
        }
    };

    mergeSortResources(arr, 0, arr.size() - 1, comp);
}

} // namespace aegis
