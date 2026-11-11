#include "resource/resource_types.hpp"
#include "resource/resource_sort.hpp"
#include "resource/knapsack_dp.hpp"
#include "resource/greedy_optimizer.hpp"
#include "resource/resource_engine.hpp"
#include <iostream>
#include <vector>
#include <cmath>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

using namespace aegis;

static void testMergeSortResources() {
    std::vector<ResourceItem> items = {
        {1, "Blanket", "Blankets", 50, 1.5, 40, 10, 0, "", 40.0 / 1.5},
        {2, "Water Jug", "Drinking Water", 100, 5.0, 90, 20, 0, "", 90.0 / 5.0},
        {3, "Med Kit", "First Aid", 30, 2.0, 85, 10, 0, "", 85.0 / 2.0},
        {4, "Canned Food", "Food", 60, 0.8, 30, 15, 0, "", 30.0 / 0.8}
    };

    // Sort by ratio descending
    sortResources(items, ResourceSortField::Ratio, false);
    // Ratios: Med Kit = 42.5, Canned Food = 37.5, Blanket = 26.67, Water Jug = 18.0
    CHECK(items[0].name == "Med Kit");
    CHECK(items[1].name == "Canned Food");
    CHECK(items[2].name == "Blanket");
    CHECK(items[3].name == "Water Jug");

    // Sort by value score descending
    sortResources(items, ResourceSortField::Value, false);
    CHECK(items[0].value_score == 90);
    CHECK(items[1].value_score == 85);
    CHECK(items[2].value_score == 40);
    CHECK(items[3].value_score == 30);

    // Sort by weight ascending
    sortResources(items, ResourceSortField::Weight, true);
    CHECK(items[0].unit_weight == 0.8);
    CHECK(items[3].unit_weight == 5.0);

    // Sort by name alphabetical
    sortResources(items, ResourceSortField::Name, true);
    CHECK(items[0].name == "Blanket");
    CHECK(items[1].name == "Canned Food");
}

static void testKnapsackOptimal() {
    // Classic knapsack scenario where Greedy fails but DP finds true optimal:
    // Capacity = 50 kg
    // Item 1: wt = 10 kg, val = 60 (ratio = 6.0)
    // Item 2: wt = 20 kg, val = 100 (ratio = 5.0)
    // Item 3: wt = 30 kg, val = 120 (ratio = 4.0)
    //
    // Greedy will pick: Item 1 (10kg, 60val), Item 2 (20kg, 100val) -> total wt 30kg, total val 160.
    // Item 3 (30kg) does not fit (30 + 30 = 60 > 50). Remaining capacity = 20kg.
    // Optimal 0/1 choice: Item 2 (20kg, 100val) + Item 3 (30kg, 120val) -> total wt 50kg, total val 220!
    // DP should achieve 220, whereas Greedy achieves 160.

    std::vector<ResourceItem> items = {
        {1, "Item A", "Equipment", 10, 10.0, 60, 0, 0, "", 6.0},
        {2, "Item B", "Food", 10, 20.0, 100, 0, 0, "", 5.0},
        {3, "Item C", "Medical", 10, 30.0, 120, 0, 0, "", 4.0}
    };

    auto dp_res = KnapsackOptimizer::solve(items, 50.0);
    auto greedy_res = GreedyOptimizer::solve(items, 50.0);

    CHECK(dp_res.total_value == 220);
    CHECK(std::abs(dp_res.total_weight - 50.0) < 1e-4);
    CHECK(dp_res.selected_items.size() == 2);
    CHECK(dp_res.selected_items[0].name == "Item B");
    CHECK(dp_res.selected_items[1].name == "Item C");

    CHECK(greedy_res.total_value == 160);
    CHECK(std::abs(greedy_res.total_weight - 30.0) < 1e-4);
    CHECK(greedy_res.selected_items.size() == 2);
    CHECK(greedy_res.selected_items[0].name == "Item A");
    CHECK(greedy_res.selected_items[1].name == "Item B");

    // Engine comparison
    ResourceEngine engine;
    auto comp = engine.compareOptimizers(items, 50.0);
    CHECK(comp.value_difference == 60);
    CHECK(comp.winner.find("Dynamic Programming") != std::string::npos);
    CHECK(!comp.analysis_summary.empty());
}

static void testEdgeCases() {
    // 1. Zero capacity
    std::vector<ResourceItem> items = {
        {1, "Item A", "Equipment", 5, 10.0, 50, 0, 0, "", 5.0}
    };
    auto dp_zero = KnapsackOptimizer::solve(items, 0.0);
    CHECK(dp_zero.total_value == 0);
    CHECK(dp_zero.selected_items.empty());

    // 2. Empty items
    std::vector<ResourceItem> empty_items;
    auto dp_empty = KnapsackOptimizer::solve(empty_items, 100.0);
    CHECK(dp_empty.total_value == 0);
    CHECK(dp_empty.selected_items.empty());

    // 3. All items heavier than capacity
    auto dp_heavy = KnapsackOptimizer::solve(items, 5.0);
    CHECK(dp_heavy.total_value == 0);
    CHECK(dp_heavy.selected_items.empty());

    // 4. Exact capacity fit
    auto dp_exact = KnapsackOptimizer::solve(items, 10.0);
    CHECK(dp_exact.total_value == 50);
    CHECK(dp_exact.selected_items.size() == 1);
}

int main() {
    std::cout << "Running Resource Optimizer Unit Tests...\n";
    testMergeSortResources();
    testKnapsackOptimal();
    testEdgeCases();

    if (failures == 0) {
        std::cout << "All Resource Optimizer tests passed successfully!\n";
        return 0;
    } else {
        std::cerr << failures << " tests failed!\n";
        return 1;
    }
}
