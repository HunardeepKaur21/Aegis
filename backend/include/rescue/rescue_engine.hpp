#pragma once
#include <memory>
#include <string>
#include <vector>
#include "rescue/dp_optimizer.hpp"
#include "rescue/merge_sort.hpp"
#include "rescue/priority_queue.hpp"
#include "rescue/request_hash_map.hpp"
#include "rescue/request_queue.hpp"
#include "rescue/rescue_types.hpp"

namespace aegis {

class RescueEngine {
public:
    RescueEngine();
    ~RescueEngine() = default;

    // Documented, configurable priority scoring formula
    static double calculatePriorityScore(
        int severityLevel,
        int affectedPeople,
        double waitingMinutes,
        const std::string& emergencyType);

    static PriorityBreakdown getPriorityBreakdown(
        int severityLevel,
        int affectedPeople,
        double waitingMinutes,
        const std::string& emergencyType);

    // Sync all requests from storage into internal DSA structures
    void loadRequests(const std::vector<EmergencyRequest>& requests);

    // DSA operations
    void addRequest(const EmergencyRequest& req);
    EmergencyRequest* findRequest(int id);
    const EmergencyRequest* findRequest(int id) const;
    bool removeRequest(int id);

    // Max Heap / Priority Queue
    bool hasUrgentRequests() const;
    EmergencyRequest popHighestPriority();
    const EmergencyRequest& peekHighestPriority() const;
    std::vector<EmergencyRequest> getHeapArray() const;
    bool updatePriority(int requestId, double newScore);

    // FIFO Queue
    std::vector<EmergencyRequest> getIncomingQueue() const;

    // Sorting (C++ Merge Sort)
    std::vector<EmergencyRequest> getSortedRequests(
        RequestSortField field = RequestSortField::PriorityScore,
        bool ascending = false) const;

    // Dynamic Programming Optimizer
    AllocationResult optimizeAllocation(int capacity) const;

    size_t count() const { return hashMap_.size(); }

private:
    RequestHashMap hashMap_;
    EmergencyPriorityQueue priorityQueue_;
    EmergencyRequestQueue fifoQueue_;
};

} // namespace aegis
