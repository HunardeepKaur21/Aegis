#pragma once
#include <cmath>
#include <functional>
#include <string>
#include <vector>
#include "rescue/rescue_types.hpp"

namespace aegis {

enum class RequestSortField {
    PriorityScore,
    WaitingTime,
    Severity,
    AffectedPeople,
    Id
};

// Generic stable merge sort
template <typename T, typename Compare>
void mergeSort(std::vector<T>& arr, size_t left, size_t right, Compare comp) {
    if (left >= right) return;
    size_t mid = left + (right - left) / 2;
    mergeSort(arr, left, mid, comp);
    mergeSort(arr, mid + 1, right, comp);

    // Merge step
    std::vector<T> temp;
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

template <typename T, typename Compare>
void mergeSort(std::vector<T>& arr, Compare comp) {
    if (arr.size() <= 1) return;
    mergeSort(arr, 0, arr.size() - 1, comp);
}

// Default emergency comparator: priority score (descending), then waiting time (descending), then id (ascending)
inline bool defaultEmergencyComparator(const EmergencyRequest& a, const EmergencyRequest& b) {
    if (std::fabs(a.priority_score - b.priority_score) > 1e-4) {
        return a.priority_score > b.priority_score;
    }
    if (std::fabs(a.waiting_minutes - b.waiting_minutes) > 1e-4) {
        return a.waiting_minutes > b.waiting_minutes;
    }
    return a.id < b.id;
}

// Stable merge sort for emergency requests by selected field
inline void sortRequests(std::vector<EmergencyRequest>& requests, RequestSortField field = RequestSortField::PriorityScore, bool ascending = false) {
    if (requests.size() <= 1) return;

    auto comp = [field, ascending](const EmergencyRequest& a, const EmergencyRequest& b) -> bool {
        switch (field) {
            case RequestSortField::PriorityScore: {
                if (std::fabs(a.priority_score - b.priority_score) > 1e-4) {
                    return ascending ? (a.priority_score < b.priority_score)
                                     : (a.priority_score > b.priority_score);
                }
                // Tie break on waiting time then id
                if (std::fabs(a.waiting_minutes - b.waiting_minutes) > 1e-4) {
                    return a.waiting_minutes > b.waiting_minutes;
                }
                return a.id < b.id;
            }
            case RequestSortField::WaitingTime: {
                if (std::fabs(a.waiting_minutes - b.waiting_minutes) > 1e-4) {
                    return ascending ? (a.waiting_minutes < b.waiting_minutes)
                                     : (a.waiting_minutes > b.waiting_minutes);
                }
                return a.priority_score > b.priority_score;
            }
            case RequestSortField::Severity: {
                if (a.severity_level != b.severity_level) {
                    return ascending ? (a.severity_level < b.severity_level)
                                     : (a.severity_level > b.severity_level);
                }
                return a.priority_score > b.priority_score;
            }
            case RequestSortField::AffectedPeople: {
                if (a.affected_people != b.affected_people) {
                    return ascending ? (a.affected_people < b.affected_people)
                                     : (a.affected_people > b.affected_people);
                }
                return a.priority_score > b.priority_score;
            }
            case RequestSortField::Id:
            default:
                return ascending ? (a.id < b.id) : (a.id > b.id);
        }
    };

    mergeSort(requests, comp);
}

} // namespace aegis
