#include "lab/lab_engine.hpp"
#include <algorithm>
#include <sstream>

namespace aegis {

std::vector<AlgorithmMeta> LabEngine::getAlgorithmCatalog() {
    return {
        // Sorting
        {"bubble", "Bubble Sort", "Sorting", "O(n²)", "O(1)", "Repeatedly steps through list, swapping adjacent elements if in wrong order.", {"run"}},
        {"selection", "Selection Sort", "Sorting", "O(n²)", "O(1)", "Finds minimum element from unsorted part and places it at beginning.", {"run"}},
        {"insertion", "Insertion Sort", "Sorting", "O(n²)", "O(1)", "Builds sorted array one item at a time by inserting in proper position.", {"run"}},
        {"merge", "Merge Sort", "Sorting", "O(n log n)", "O(n)", "Divide and conquer algorithm that splits array into halves, sorts and merges.", {"run"}},
        {"quick", "Quick Sort", "Sorting", "O(n log n)", "O(log n)", "Partitions array around pivot such that smaller elements precede it.", {"run"}},
        {"heap_sort", "Heap Sort", "Sorting", "O(n log n)", "O(1)", "Builds binary max heap and repeatedly extracts maximum element.", {"run"}},
        {"counting", "Counting Sort", "Sorting", "O(n + k)", "O(k)", "Non-comparative integer sorting algorithm counting occurrences of each value.", {"run"}},
        {"radix", "Radix Sort", "Sorting", "O(d · (n + b))", "O(n + b)", "Processes integer digits from least to most significant.", {"run"}},
        {"bucket", "Bucket Sort", "Sorting", "O(n + k)", "O(n)", "Distributes elements into buckets then sorts buckets individually.", {"run"}},

        // Searching
        {"linear_search", "Linear Search", "Searching", "O(n)", "O(1)", "Sequentially checks every element in list until match is found.", {"search"}},
        {"binary_search", "Binary Search", "Searching", "O(log n)", "O(1)", "Finds target in sorted array by repeatedly halving search interval.", {"search"}},
        {"recursive_binary", "Recursive Binary Search", "Searching", "O(log n)", "O(log n)", "Divide and conquer recursive binary search implementation.", {"search"}},

        // Dynamic Programming
        {"fib_memo", "Fibonacci (Memoization)", "Dynamic Programming", "O(n)", "O(n)", "Top-down recursive calculation with cache table.", {"compute"}},
        {"fib_tab", "Fibonacci (Tabulation)", "Dynamic Programming", "O(n)", "O(n)", "Bottom-up iterative calculation in linear time.", {"compute"}},
        {"knapsack_01", "0/1 Knapsack", "Dynamic Programming", "O(n · W)", "O(n · W)", "Selects optimal item subset under capacity constraint.", {"compute"}},
        {"unbounded_knapsack", "Unbounded Knapsack", "Dynamic Programming", "O(n · W)", "O(W)", "Allows unlimited copies of each item to maximize value.", {"compute"}},
        {"lcs", "Longest Common Subsequence", "Dynamic Programming", "O(m · n)", "O(m · n)", "Finds longest subsequence present in both strings in same order.", {"compute"}},
        {"lis", "Longest Increasing Subsequence", "Dynamic Programming", "O(n²)", "O(n)", "Finds longest strictly increasing subsequence in array.", {"compute"}},
        {"coin_change", "Coin Change", "Dynamic Programming", "O(n · S)", "O(S)", "Finds minimum coins needed to make change for amount.", {"compute"}},
        {"mcm", "Matrix Chain Multiplication", "Dynamic Programming", "O(n³)", "O(n²)", "Finds most efficient parenthesization order to multiply matrices.", {"compute"}},

        // Data Structures
        {"arrays", "Arrays", "Arrays", "O(1) access, O(n) insert/delete", "O(n)", "Contiguous memory sequence supporting index-based operations.", {"insert", "delete", "update", "traverse"}},
        {"strings", "Strings", "Strings", "O(n) - O(n + m)", "O(m)", "Text processing: KMP match, anagrams, palindrome, frequencies.", {"reverse", "palindrome", "anagram", "frequency", "kmp"}},
        {"stacks", "Stacks", "Stacks", "O(1)", "O(n)", "LIFO stack visualizer supporting push, pop and peek.", {"push", "pop", "peek"}},
        {"queues", "Queues", "Queues", "O(1)", "O(n)", "FIFO queue visualizer supporting enqueue and dequeue.", {"enqueue", "dequeue", "peek"}},
        {"trees", "Trees", "Trees", "O(n)", "O(h)", "Binary tree traversals: preorder, inorder, postorder, level order.", {"preorder", "inorder", "postorder", "levelorder"}},
        {"hashing", "Hashing", "Hashing", "O(1) avg", "O(m)", "Hash collision resolutions: Separate Chaining and Linear Probing.", {"chaining", "probing"}},
        {"heaps", "Heaps", "Heaps", "O(log n)", "O(n)", "Binary Min and Max Heaps with visual heapify array.", {"min_heap", "max_heap"}}
    };
}

LabRunResult LabEngine::execute(
    const std::string& category,
    const std::string& algorithm,
    const std::vector<int>& input_data,
    const std::string& operation,
    int param_val,
    const std::string& str_param1,
    const std::string& str_param2)
{
    std::string catLower = category;
    std::transform(catLower.begin(), catLower.end(), catLower.begin(), ::tolower);

    // 1. Sorting
    if (catLower == "sorting") {
        return LabSorting::run(algorithm, input_data);
    }

    // 2. Searching
    if (catLower == "searching") {
        return LabSearching::run(algorithm, input_data, param_val);
    }

    // 3. Dynamic Programming
    if (catLower == "dynamic programming" || catLower == "dp") {
        std::string algoLower = algorithm;
        std::transform(algoLower.begin(), algoLower.end(), algoLower.begin(), ::tolower);

        if (algoLower.find("memo") != std::string::npos) {
            return LabDP::fibonacciMemo(param_val > 0 ? param_val : 10);
        }
        if (algoLower.find("tab") != std::string::npos || algoLower.find("fibonacci") != std::string::npos) {
            return LabDP::fibonacciTab(param_val > 0 ? param_val : 10);
        }
        if (algoLower.find("0/1") != std::string::npos || algoLower.find("knapsack") != std::string::npos) {
            std::vector<int> weights = {2, 3, 4, 5};
            std::vector<int> values = {3, 4, 5, 8};
            return LabDP::knapsack01(weights, values, param_val > 0 ? param_val : 8);
        }
        if (algoLower.find("unbounded") != std::string::npos) {
            std::vector<int> weights = {1, 3, 4};
            std::vector<int> values = {15, 50, 60};
            return LabDP::knapsackUnbounded(weights, values, param_val > 0 ? param_val : 8);
        }
        if (algoLower.find("lcs") != std::string::npos) {
            std::string s1 = str_param1.empty() ? "AEDFHR" : str_param1;
            std::string s2 = str_param2.empty() ? "ABCDGH" : str_param2;
            return LabDP::lcs(s1, s2);
        }
        if (algoLower.find("lis") != std::string::npos) {
            std::vector<int> arr = input_data.empty() ? std::vector<int>{10, 22, 9, 33, 21, 50, 41, 60} : input_data;
            return LabDP::lis(arr);
        }
        if (algoLower.find("coin") != std::string::npos) {
            std::vector<int> coins = input_data.empty() ? std::vector<int>{1, 2, 5} : input_data;
            return LabDP::coinChange(coins, param_val > 0 ? param_val : 11);
        }
        if (algoLower.find("mcm") != std::string::npos || algoLower.find("matrix") != std::string::npos) {
            std::vector<int> dims = input_data.empty() ? std::vector<int>{10, 20, 30, 40, 30} : input_data;
            return LabDP::matrixChain(dims);
        }
        return LabDP::fibonacciTab(10);
    }

    // 4. Arrays
    if (catLower == "arrays") {
        return LabStructures::runArray(input_data, operation, param_val, 99);
    }

    // 5. Strings
    if (catLower == "strings") {
        return LabStructures::runString(str_param1, str_param2, operation);
    }

    // 6. Stacks
    if (catLower == "stacks") {
        return LabStructures::runStack(input_data, operation, param_val);
    }

    // 7. Queues
    if (catLower == "queues") {
        return LabStructures::runQueue(input_data, operation, param_val);
    }

    // 8. Trees
    if (catLower == "trees") {
        return LabStructures::runTree(input_data, operation.empty() ? "inorder" : operation);
    }

    // 9. Hashing
    if (catLower == "hashing") {
        return LabStructures::runHashing(input_data, operation.empty() ? "chaining" : operation, param_val > 0 ? param_val : 10);
    }

    // 10. Heaps
    if (catLower == "heaps") {
        return LabStructures::runHeap(input_data, operation != "min_heap");
    }

    // Default fallback
    return LabSorting::run(algorithm, input_data);
}

LabCompareResult LabEngine::compare(
    const std::string& algo_a,
    const std::string& algo_b,
    const std::vector<int>& input_data)
{
    LabCompareResult comp;
    comp.algorithm_a = algo_a;
    comp.algorithm_b = algo_b;

    comp.result_a = execute("Sorting", algo_a, input_data);
    comp.result_b = execute("Sorting", algo_b, input_data);

    std::ostringstream ss;
    if (comp.result_a.runtime_us < comp.result_b.runtime_us) {
        comp.winner = algo_a;
        ss << algo_a << " completed faster (" << comp.result_a.runtime_us << " μs vs " 
           << comp.result_b.runtime_us << " μs) with " << comp.result_a.comparisons << " comparisons.";
    } else if (comp.result_a.runtime_us > comp.result_b.runtime_us) {
        comp.winner = algo_b;
        ss << algo_b << " completed faster (" << comp.result_b.runtime_us << " μs vs " 
           << comp.result_a.runtime_us << " μs) with " << comp.result_b.comparisons << " comparisons.";
    } else {
        comp.winner = "Tie";
        ss << "Both algorithms executed in identical duration.";
    }
    comp.summary = ss.str();
    return comp;
}

} // namespace aegis
