#pragma once
#include <cmath>
#include "rescue/max_heap.hpp"
#include "rescue/rescue_types.hpp"

namespace aegis {

struct RequestKeyExtractor {
    int operator()(const EmergencyRequest& req) const {
        return req.id;
    }
};

struct EmergencyPriorityComparator {
    bool operator()(const EmergencyRequest& a, const EmergencyRequest& b) const {
        // 1. Higher priority score comes first
        if (std::fabs(a.priority_score - b.priority_score) > 1e-4) {
            return a.priority_score > b.priority_score;
        }
        // 2. Tie-break: Longer waiting time comes first
        if (std::fabs(a.waiting_minutes - b.waiting_minutes) > 1e-4) {
            return a.waiting_minutes > b.waiting_minutes;
        }
        // 3. Tie-break: Lower ID (earlier created) comes first
        return a.id < b.id;
    }
};

// Priority Queue processing emergency requests according to priority scores
class EmergencyPriorityQueue {
public:
    EmergencyPriorityQueue()
        : heap_(RequestKeyExtractor{}, EmergencyPriorityComparator{}) {}

    void insert(const EmergencyRequest& req) {
        heap_.push(req);
    }

    EmergencyRequest extractHighestPriority() {
        return heap_.pop();
    }

    const EmergencyRequest& peekHighestPriority() const {
        return heap_.top();
    }

    bool updatePriority(int requestId, double newScore) {
        return heap_.updatePriority(requestId, [newScore](EmergencyRequest& req) {
            req.priority_score = newScore;
        });
    }

    bool empty() const { return heap_.empty(); }
    size_t size() const { return heap_.size(); }
    void clear() { heap_.clear(); }

    std::vector<EmergencyRequest> displayHeap() const {
        return heap_.displayHeap();
    }

    void buildQueue(const std::vector<EmergencyRequest>& requests) {
        heap_.buildHeap(requests);
    }

private:
    MaxHeap<EmergencyRequest, RequestKeyExtractor, EmergencyPriorityComparator> heap_;
};

} // namespace aegis
