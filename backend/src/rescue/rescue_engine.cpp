#include "rescue/rescue_engine.hpp"
#include <algorithm>
#include <cmath>

namespace aegis {

RescueEngine::RescueEngine() = default;

double RescueEngine::calculatePriorityScore(
    int severityLevel,
    int affectedPeople,
    double waitingMinutes,
    const std::string& emergencyType)
{
    return getPriorityBreakdown(severityLevel, affectedPeople, waitingMinutes, emergencyType).total_score;
}

PriorityBreakdown RescueEngine::getPriorityBreakdown(
    int severityLevel,
    int affectedPeople,
    double waitingMinutes,
    const std::string& emergencyType)
{
    PriorityBreakdown b;
    // 1. Severity component (base weight max 50.0)
    if (severityLevel >= 4) {
        b.severity_component = 50.0;
    } else if (severityLevel == 3) {
        b.severity_component = 35.0;
    } else if (severityLevel == 2) {
        b.severity_component = 20.0;
    } else {
        b.severity_component = 10.0;
    }

    // 2. Affected people component (capped at 30.0)
    b.people_component = std::min(30.0, std::max(0, affectedPeople) * 1.5);

    // 3. Waiting time component (capped at 20.0, 0.2 points per minute)
    b.waiting_component = std::min(20.0, std::max(0.0, waitingMinutes) * 0.2);

    // 4. Emergency type specific bonus
    if (emergencyType == "Fire" || emergencyType == "fire") {
        b.type_bonus = 5.0;
    } else if (emergencyType == "Flood" || emergencyType == "flood") {
        b.type_bonus = 5.0;
    } else if (emergencyType == "Building Collapse" || emergencyType == "building collapse") {
        b.type_bonus = 5.0;
    } else if (emergencyType == "Medical" || emergencyType == "medical") {
        b.type_bonus = 4.0;
    } else if (emergencyType == "Accident" || emergencyType == "accident") {
        b.type_bonus = 3.0;
    } else {
        b.type_bonus = 0.0;
    }

    b.total_score = std::round((b.severity_component + b.people_component + b.waiting_component + b.type_bonus) * 10.0) / 10.0;
    return b;
}

void RescueEngine::loadRequests(const std::vector<EmergencyRequest>& requests) {
    hashMap_.clear();
    priorityQueue_.clear();
    fifoQueue_.clear();

    for (const auto& req : requests) {
        hashMap_.put(req.id, req);
        fifoQueue_.enqueue(req);
        if (req.status == "pending") {
            priorityQueue_.insert(req);
        }
    }
}

void RescueEngine::addRequest(const EmergencyRequest& req) {
    hashMap_.put(req.id, req);
    fifoQueue_.enqueue(req);
    if (req.status == "pending") {
        priorityQueue_.insert(req);
    }
}

EmergencyRequest* RescueEngine::findRequest(int id) {
    return hashMap_.get(id);
}

const EmergencyRequest* RescueEngine::findRequest(int id) const {
    return hashMap_.get(id);
}

bool RescueEngine::removeRequest(int id) {
    return hashMap_.remove(id);
}

bool RescueEngine::hasUrgentRequests() const {
    return !priorityQueue_.empty();
}

EmergencyRequest RescueEngine::popHighestPriority() {
    return priorityQueue_.extractHighestPriority();
}

const EmergencyRequest& RescueEngine::peekHighestPriority() const {
    return priorityQueue_.peekHighestPriority();
}

std::vector<EmergencyRequest> RescueEngine::getHeapArray() const {
    return priorityQueue_.displayHeap();
}

bool RescueEngine::updatePriority(int requestId, double newScore) {
    bool heapUpdated = priorityQueue_.updatePriority(requestId, newScore);
    EmergencyRequest* req = hashMap_.get(requestId);
    if (req) {
        req->priority_score = newScore;
    }
    return heapUpdated || (req != nullptr);
}

std::vector<EmergencyRequest> RescueEngine::getIncomingQueue() const {
    return fifoQueue_.toList();
}

std::vector<EmergencyRequest> RescueEngine::getSortedRequests(
    RequestSortField field,
    bool ascending) const
{
    std::vector<EmergencyRequest> all = hashMap_.values();
    sortRequests(all, field, ascending);
    return all;
}

AllocationResult RescueEngine::optimizeAllocation(int capacity) const {
    std::vector<EmergencyRequest> pendingCandidates;
    for (const auto& req : hashMap_.values()) {
        if (req.status == "pending") {
            pendingCandidates.push_back(req);
        }
    }
    return optimizeRescueAllocation(pendingCandidates, capacity);
}

} // namespace aegis
