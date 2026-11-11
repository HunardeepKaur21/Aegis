#include "rescue/dp_optimizer.hpp"
#include "rescue/max_heap.hpp"
#include "rescue/merge_sort.hpp"
#include "rescue/priority_queue.hpp"
#include "rescue/request_hash_map.hpp"
#include "rescue/request_queue.hpp"
#include "rescue/rescue_engine.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

using namespace aegis;

static void testMaxHeap() {
    struct TestItem {
        int id;
        double score;
    };
    struct KeyExt {
        int operator()(const TestItem& i) const { return i.id; }
    };
    struct Comp {
        bool operator()(const TestItem& a, const TestItem& b) const { return a.score > b.score; }
    };

    MaxHeap<TestItem, KeyExt, Comp> heap(KeyExt{}, Comp{});
    CHECK(heap.empty());

    heap.push({1, 10.5});
    heap.push({2, 50.0});
    heap.push({3, 30.2});
    heap.push({4, 99.9});
    heap.push({5, 5.0});

    CHECK(heap.size() == 5);
    CHECK(heap.top().id == 4);
    CHECK(std::fabs(heap.top().score - 99.9) < 1e-4);

    // Test updatePriority: increase id 1 from 10.5 to 105.0 -> should become root
    bool updated = heap.updatePriority(1, [](TestItem& item) { item.score = 105.0; });
    CHECK(updated);
    CHECK(heap.top().id == 1);
    CHECK(std::fabs(heap.top().score - 105.0) < 1e-4);

    // Extract elements in order
    double prev = 10000.0;
    while (!heap.empty()) {
        TestItem top = heap.pop();
        CHECK(top.score <= prev);
        prev = top.score;
    }
    CHECK(heap.empty());
}

static void testPriorityQueue() {
    EmergencyPriorityQueue pq;
    EmergencyRequest r1; r1.id = 1; r1.priority_score = 40.0; r1.waiting_minutes = 10.0;
    EmergencyRequest r2; r2.id = 2; r2.priority_score = 80.0; r2.waiting_minutes = 5.0;
    EmergencyRequest r3; r3.id = 3; r3.priority_score = 80.0; r3.waiting_minutes = 25.0; // Same score, higher wait
    EmergencyRequest r4; r4.id = 4; r4.priority_score = 80.0; r4.waiting_minutes = 25.0; // Same score & wait, higher id

    pq.insert(r1);
    pq.insert(r2);
    pq.insert(r3);
    pq.insert(r4);

    CHECK(pq.size() == 4);
    // r3 should come before r4 (tie broken by id: 3 < 4) and before r2 (wait: 25 > 5)
    EmergencyRequest top1 = pq.extractHighestPriority();
    CHECK(top1.id == 3);

    EmergencyRequest top2 = pq.extractHighestPriority();
    CHECK(top2.id == 4);

    EmergencyRequest top3 = pq.extractHighestPriority();
    CHECK(top3.id == 2);

    EmergencyRequest top4 = pq.extractHighestPriority();
    CHECK(top4.id == 1);
}

static void testHashMap() {
    RequestHashMap map;
    CHECK(map.empty());
    CHECK(map.size() == 0);

    for (int i = 1; i <= 100; ++i) {
        EmergencyRequest r;
        r.id = i;
        r.victim_name = "Victim " + std::to_string(i);
        r.priority_score = static_cast<double>(i * 2);
        map.put(i, r);
    }

    CHECK(map.size() == 100);
    CHECK(map.has(42));
    CHECK(!map.has(999));

    EmergencyRequest* req = map.get(42);
    CHECK(req != nullptr);
    CHECK(req->victim_name == "Victim 42");

    // Update existing key
    EmergencyRequest updated;
    updated.id = 42;
    updated.victim_name = "Updated Victim 42";
    map.put(42, updated);
    CHECK(map.size() == 100);
    CHECK(map.get(42)->victim_name == "Updated Victim 42");

    // Remove
    CHECK(map.remove(42));
    CHECK(!map.has(42));
    CHECK(map.size() == 99);
    CHECK(!map.remove(42)); // Second removal fails
}

static void testMergeSort() {
    std::vector<EmergencyRequest> reqs;
    EmergencyRequest r1; r1.id = 1; r1.priority_score = 45.0; r1.waiting_minutes = 12.0; r1.affected_people = 10;
    EmergencyRequest r2; r2.id = 2; r2.priority_score = 90.0; r2.waiting_minutes = 5.0;  r2.affected_people = 2;
    EmergencyRequest r3; r3.id = 3; r3.priority_score = 70.0; r3.waiting_minutes = 30.0; r3.affected_people = 50;
    EmergencyRequest r4; r4.id = 4; r4.priority_score = 90.0; r4.waiting_minutes = 20.0; r4.affected_people = 1;

    reqs = {r1, r2, r3, r4};
    sortRequests(reqs, RequestSortField::PriorityScore, false);

    // r4 (90, wait 20) -> r2 (90, wait 5) -> r3 (70) -> r1 (45)
    CHECK(reqs[0].id == 4);
    CHECK(reqs[1].id == 2);
    CHECK(reqs[2].id == 3);
    CHECK(reqs[3].id == 1);

    // Sort by affected people ascending
    sortRequests(reqs, RequestSortField::AffectedPeople, true);
    CHECK(reqs[0].affected_people == 1);
    CHECK(reqs[1].affected_people == 2);
    CHECK(reqs[2].affected_people == 10);
    CHECK(reqs[3].affected_people == 50);
}

static void testFIFOQueue() {
    EmergencyRequestQueue q;
    CHECK(q.empty());

    for (int i = 1; i <= 5; ++i) {
        EmergencyRequest r;
        r.id = i;
        q.enqueue(r);
    }
    CHECK(q.size() == 5);
    CHECK(q.front().id == 1);

    for (int i = 1; i <= 5; ++i) {
        EmergencyRequest r = q.dequeue();
        CHECK(r.id == i);
    }
    CHECK(q.empty());
}

static void testDPOptimizer() {
    std::vector<EmergencyRequest> candidates;
    // Request 1: 5 people, priority 50 (ratio 10)
    EmergencyRequest r1; r1.id = 1; r1.affected_people = 5; r1.priority_score = 50.0;
    // Request 2: 10 people, priority 60 (ratio 6)
    EmergencyRequest r2; r2.id = 2; r2.affected_people = 10; r2.priority_score = 60.0;
    // Request 3: 8 people, priority 70 (ratio 8.75)
    EmergencyRequest r3; r3.id = 3; r3.affected_people = 8; r3.priority_score = 70.0;
    // Request 4: 3 people, priority 35 (ratio 11.67)
    EmergencyRequest r4; r4.id = 4; r4.affected_people = 3; r4.priority_score = 35.0;

    candidates = {r1, r2, r3, r4};

    // Capacity = 16: optimal combination is r1(5) + r3(8) + r4(3) = 16 people, priority = 50 + 70 + 35 = 155.0
    AllocationResult res = optimizeRescueAllocation(candidates, 16);
    CHECK(res.total_capacity == 16);
    CHECK(res.used_capacity == 16);
    CHECK(std::fabs(res.total_priority_score - 155.0) < 1e-4);
    CHECK(res.selected_requests.size() == 3);
}

static void testScoringFormula() {
    // Critical (4), 20 people, 30 mins, Fire
    // Base: 50
    // People: min(30, 20 * 1.5) = 30
    // Waiting: min(20, 30 * 0.2) = 6
    // Fire bonus: 5
    // Total = 50 + 30 + 6 + 5 = 91.0
    double score = RescueEngine::calculatePriorityScore(4, 20, 30.0, "Fire");
    CHECK(std::fabs(score - 91.0) < 1e-4);

    PriorityBreakdown b = RescueEngine::getPriorityBreakdown(4, 20, 30.0, "Fire");
    CHECK(std::fabs(b.severity_component - 50.0) < 1e-4);
    CHECK(std::fabs(b.people_component - 30.0) < 1e-4);
    CHECK(std::fabs(b.waiting_component - 6.0) < 1e-4);
    CHECK(std::fabs(b.type_bonus - 5.0) < 1e-4);
    CHECK(std::fabs(b.total_score - 91.0) < 1e-4);
}

int main() {
    testMaxHeap();
    testPriorityQueue();
    testHashMap();
    testMergeSort();
    testFIFOQueue();
    testDPOptimizer();
    testScoringFormula();

    if (failures == 0) {
        std::cout << "test_rescue: all DSA tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
