#include "lab/lab_types.hpp"
#include "lab/lab_sorting.hpp"
#include "lab/lab_searching.hpp"
#include "lab/lab_dp.hpp"
#include "lab/lab_structures.hpp"
#include "lab/lab_engine.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { ++failures; std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)

using namespace aegis;

static void testAllSortingAlgorithms() {
    std::vector<int> original = {38, 27, 43, 3, 9, 82, 10, 19, 50, 1};
    std::vector<int> expected = original;
    std::sort(expected.begin(), expected.end());

    std::vector<std::string> algorithms = {
        "bubble", "selection", "insertion", "merge", "quick", "heap", "counting", "radix", "bucket"
    };

    for (const auto& algo : algorithms) {
        auto res = LabSorting::run(algo, original);
        CHECK(res.result_array == expected);
        CHECK(!res.steps.empty());
        CHECK(res.runtime_us >= 0.0);
    }
}

static void testSearchingAlgorithms() {
    std::vector<int> arr = {5, 12, 18, 23, 39, 45, 67, 89, 94};

    // Linear Search
    auto ls_found = LabSearching::run("linear", arr, 45);
    CHECK(ls_found.success);
    CHECK(ls_found.output_value.find("index 5") != std::string::npos);

    auto ls_missing = LabSearching::run("linear", arr, 999);
    CHECK(!ls_missing.success);

    // Binary Search
    auto bs_found = LabSearching::run("binary", arr, 67);
    CHECK(bs_found.success);
    CHECK(bs_found.output_value.find("index 6") != std::string::npos);

    auto bs_missing = LabSearching::run("binary", arr, 1);
    CHECK(!bs_missing.success);

    // Recursive Binary Search
    auto rbs_found = LabSearching::run("recursive binary", arr, 23);
    CHECK(rbs_found.success);
    CHECK(rbs_found.output_value.find("index 3") != std::string::npos);
}

static void testDPAlgorithms() {
    // 1. Fibonacci
    auto fibM = LabDP::fibonacciMemo(10);
    auto fibT = LabDP::fibonacciTab(10);
    CHECK(fibM.output_value == "55");
    CHECK(fibT.output_value == "55");

    // 2. 0/1 Knapsack
    // weights = [2, 3, 4, 5], values = [3, 4, 5, 8], W = 8 -> optimal 12
    auto knap01 = LabDP::knapsack01({2, 3, 4, 5}, {3, 4, 5, 8}, 8);
    CHECK(knap01.output_value == "12");

    // 3. Unbounded Knapsack
    // weights = [1, 3, 4], values = [15, 50, 60], W = 8 -> pick 2x4 = 120 or 2x3 + 2x1 = 130
    auto knapUn = LabDP::knapsackUnbounded({1, 3, 4}, {15, 50, 60}, 8);
    CHECK(std::stoi(knapUn.output_value) >= 120);

    // 4. LCS
    auto lcs_res = LabDP::lcs("AGGTAB", "GXTXAYB");
    CHECK(lcs_res.output_value.find("GTAB") != std::string::npos);

    // 5. LIS
    auto lis_res = LabDP::lis({10, 22, 9, 33, 21, 50, 41, 60, 80});
    CHECK(lis_res.output_value == "6"); // [10, 22, 33, 50, 60, 80]

    // 6. Coin Change
    auto coin_res = LabDP::coinChange({1, 2, 5}, 11);
    CHECK(coin_res.output_value.find("3 coins") != std::string::npos); // 5 + 5 + 1

    // 7. MCM
    auto mcm_res = LabDP::matrixChain({10, 20, 30, 40, 30});
    CHECK(std::stoi(mcm_res.output_value) > 0);
}

static void testDataStructures() {
    // String KMP
    auto kmp_res = LabStructures::runString("ABABDABACDABABCABAB", "ABABCABAB", "kmp");
    CHECK(kmp_res.result_array.size() == 1);
    CHECK(kmp_res.result_array[0] == 10);

    // String Anagram & Palindrome
    auto ana = LabStructures::runString("listen", "silent", "anagram");
    CHECK(ana.output_value.find("TRUE") != std::string::npos);

    auto pal = LabStructures::runString("racecar", "", "palindrome");
    CHECK(pal.output_value.find("TRUE") != std::string::npos);

    // Stack
    auto st_push = LabStructures::runStack({1, 2, 3}, "push", 4);
    CHECK(st_push.result_array.size() == 4);
    auto st_pop = LabStructures::runStack({1, 2, 3}, "pop", 0);
    CHECK(st_pop.output_value == "3");

    // Queue
    auto q_enq = LabStructures::runQueue({10, 20}, "enqueue", 30);
    CHECK(q_enq.result_array.size() == 3);
    auto q_deq = LabStructures::runQueue({10, 20}, "dequeue", 0);
    CHECK(q_deq.output_value == "10");

    // Trees
    auto tree_in = LabStructures::runTree({1, 2, 3, 4, 5}, "inorder");
    CHECK(tree_in.result_array.size() == 5);

    // Heaps
    auto max_hp = LabStructures::runHeap({10, 30, 20, 5, 40}, true);
    CHECK(max_hp.result_array[0] == 40);

    // Hashing
    auto hash_res = LabStructures::runHashing({12, 22, 32, 5, 15}, "chaining", 10);
    CHECK(hash_res.key_value_pairs.size() == 10);
}

int main() {
    std::cout << "Running Complete DSA Algorithm Laboratory Unit Tests...\n";
    testAllSortingAlgorithms();
    testSearchingAlgorithms();
    testDPAlgorithms();
    testDataStructures();

    if (failures == 0) {
        std::cout << "All Algorithm Laboratory tests passed successfully!\n";
        return 0;
    } else {
        std::cerr << failures << " tests failed!\n";
        return 1;
    }
}
