#pragma once
#include <algorithm>
#include <chrono>
#include <climits>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include "lab/lab_types.hpp"

namespace aegis {

class LabDP {
public:
    // 1. Fibonacci (Memoization)
    static LabRunResult fibonacciMemo(int n) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Dynamic Programming";
        res.algorithm = "Fibonacci (Memoization)";
        res.time_complexity = "O(n)";
        res.space_complexity = "O(n)";

        if (n < 0) n = 0;
        if (n > 40) n = 40; // cap for 32-bit demo

        std::vector<long long> memo(n + 1, -1);
        std::function<long long(int)> fib = [&](int k) -> long long {
            res.comparisons++;
            if (k <= 0) return 0;
            if (k == 1) return 1;
            if (memo[k] != -1) return memo[k];
            memo[k] = fib(k - 1) + fib(k - 2);
            return memo[k];
        };

        long long ans = fib(n);
        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        res.output_value = std::to_string(ans);
        res.message = "Fibonacci(" + std::to_string(n) + ") = " + std::to_string(ans) + " (Top-down memoized recursion)";

        // Table
        res.row_headers = {"F(k)"};
        std::vector<int> row;
        for (int i = 0; i <= n; ++i) {
            res.col_headers.push_back(std::to_string(i));
            row.push_back(static_cast<int>(memo[i]));
        }
        res.matrix_data = {row};
        return res;
    }

    // 2. Fibonacci (Tabulation)
    static LabRunResult fibonacciTab(int n) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Dynamic Programming";
        res.algorithm = "Fibonacci (Tabulation)";
        res.time_complexity = "O(n)";
        res.space_complexity = "O(n)";

        if (n < 0) n = 0;
        if (n > 40) n = 40;

        std::vector<long long> dp(n + 1, 0);
        if (n >= 1) dp[1] = 1;

        for (int i = 2; i <= n; ++i) {
            dp[i] = dp[i - 1] + dp[i - 2];
            res.swaps++;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        res.output_value = std::to_string(dp[n]);
        res.message = "Fibonacci(" + std::to_string(n) + ") = " + std::to_string(dp[n]) + " (Bottom-up iterative tabulation)";

        res.row_headers = {"F(k)"};
        std::vector<int> row;
        for (int i = 0; i <= n; ++i) {
            res.col_headers.push_back(std::to_string(i));
            row.push_back(static_cast<int>(dp[i]));
        }
        res.matrix_data = {row};
        return res;
    }

    // 3. 0/1 Knapsack
    static LabRunResult knapsack01(const std::vector<int>& weights, const std::vector<int>& values, int W) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Dynamic Programming";
        res.algorithm = "0/1 Knapsack";
        res.time_complexity = "O(n · W)";
        res.space_complexity = "O(n · W)";

        int n = static_cast<int>(weights.size());
        if (W > 50) W = 50; // visual cap

        std::vector<std::vector<int>> dp(n + 1, std::vector<int>(W + 1, 0));

        for (int i = 1; i <= n; ++i) {
            int wt = weights[i - 1];
            int val = values[i - 1];
            for (int w = 0; w <= W; ++w) {
                res.comparisons++;
                if (wt <= w) {
                    dp[i][w] = std::max(dp[i - 1][w], dp[i - 1][w - wt] + val);
                } else {
                    dp[i][w] = dp[i - 1][w];
                }
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        res.output_value = std::to_string(dp[n][W]);
        res.message = "Maximum priority value captured: " + std::to_string(dp[n][W]);

        res.row_headers.push_back("Base (0)");
        for (int i = 0; i < n; ++i) {
            res.row_headers.push_back("Item " + std::to_string(i + 1) + " (w=" + std::to_string(weights[i]) + ", v=" + std::to_string(values[i]) + ")");
        }
        for (int w = 0; w <= W; ++w) {
            res.col_headers.push_back(std::to_string(w));
        }
        res.matrix_data = dp;
        return res;
    }

    // 4. Unbounded Knapsack
    static LabRunResult knapsackUnbounded(const std::vector<int>& weights, const std::vector<int>& values, int W) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Dynamic Programming";
        res.algorithm = "Unbounded Knapsack";
        res.time_complexity = "O(n · W)";
        res.space_complexity = "O(W)";

        if (W > 60) W = 60;
        int n = static_cast<int>(weights.size());
        std::vector<int> dp(W + 1, 0);

        for (int w = 0; w <= W; ++w) {
            for (int i = 0; i < n; ++i) {
                res.comparisons++;
                if (weights[i] <= w) {
                    dp[w] = std::max(dp[w], dp[w - weights[i]] + values[i]);
                }
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        res.output_value = std::to_string(dp[W]);
        res.message = "Unbounded Knapsack Max Value: " + std::to_string(dp[W]);

        res.row_headers = {"Max Value"};
        for (int w = 0; w <= W; ++w) res.col_headers.push_back(std::to_string(w));
        res.matrix_data = {dp};
        return res;
    }

    // 5. Longest Common Subsequence (LCS)
    static LabRunResult lcs(const std::string& s1, const std::string& s2) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Dynamic Programming";
        res.algorithm = "Longest Common Subsequence";
        res.time_complexity = "O(m · n)";
        res.space_complexity = "O(m · n)";

        int m = static_cast<int>(s1.size());
        int n = static_cast<int>(s2.size());
        std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));

        for (int i = 1; i <= m; ++i) {
            for (int j = 1; j <= n; ++j) {
                res.comparisons++;
                if (s1[i - 1] == s2[j - 1]) {
                    dp[i][j] = 1 + dp[i - 1][j - 1];
                } else {
                    dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
                }
            }
        }

        // Backtrack LCS string
        std::string lcs_str = "";
        int i = m, j = n;
        while (i > 0 && j > 0) {
            if (s1[i - 1] == s2[j - 1]) {
                lcs_str.push_back(s1[i - 1]);
                --i; --j;
            } else if (dp[i - 1][j] > dp[i][j - 1]) {
                --i;
            } else {
                --j;
            }
        }
        std::reverse(lcs_str.begin(), lcs_str.end());

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        res.output_value = "Length: " + std::to_string(dp[m][n]) + " ('" + lcs_str + "')";
        res.message = "LCS string: '" + lcs_str + "' (Length: " + std::to_string(dp[m][n]) + ")";

        res.row_headers.push_back("Ø");
        for (char c : s1) res.row_headers.push_back(std::string(1, c));
        res.col_headers.push_back("Ø");
        for (char c : s2) res.col_headers.push_back(std::string(1, c));
        res.matrix_data = dp;
        return res;
    }

    // 6. Longest Increasing Subsequence (LIS)
    static LabRunResult lis(const std::vector<int>& arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Dynamic Programming";
        res.algorithm = "Longest Increasing Subsequence";
        res.time_complexity = "O(n²)";
        res.space_complexity = "O(n)";

        int n = static_cast<int>(arr.size());
        if (n == 0) return res;

        std::vector<int> dp(n, 1);
        int max_lis = 1;

        for (int i = 1; i < n; ++i) {
            for (int j = 0; j < i; ++j) {
                res.comparisons++;
                if (arr[i] > arr[j]) {
                    dp[i] = std::max(dp[i], dp[j] + 1);
                }
            }
            max_lis = std::max(max_lis, dp[i]);
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        res.output_value = std::to_string(max_lis);
        res.message = "Length of Longest Increasing Subsequence: " + std::to_string(max_lis);

        res.row_headers = {"LIS(i)"};
        for (int i = 0; i < n; ++i) {
            res.col_headers.push_back(std::to_string(arr[i]));
        }
        res.matrix_data = {dp};
        return res;
    }

    // 7. Coin Change (Minimum Coins)
    static LabRunResult coinChange(const std::vector<int>& coins, int amount) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Dynamic Programming";
        res.algorithm = "Coin Change";
        res.time_complexity = "O(coins · amount)";
        res.space_complexity = "O(amount)";

        if (amount > 50) amount = 50;
        std::vector<int> dp(amount + 1, amount + 1);
        dp[0] = 0;

        for (int a = 1; a <= amount; ++a) {
            for (int c : coins) {
                res.comparisons++;
                if (a - c >= 0) {
                    dp[a] = std::min(dp[a], 1 + dp[a - c]);
                }
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        int ans = dp[amount] > amount ? -1 : dp[amount];
        res.output_value = ans == -1 ? "Impossible" : std::to_string(ans) + " coins";
        res.message = ans == -1 
            ? "Cannot make exact change for amount " + std::to_string(amount)
            : "Minimum coins required for amount " + std::to_string(amount) + ": " + std::to_string(ans);

        res.row_headers = {"Min Coins"};
        std::vector<int> row;
        for (int a = 0; a <= amount; ++a) {
            res.col_headers.push_back(std::to_string(a));
            row.push_back(dp[a] > amount ? -1 : dp[a]);
        }
        res.matrix_data = {row};
        return res;
    }

    // 8. Matrix Chain Multiplication (MCM)
    static LabRunResult matrixChain(const std::vector<int>& dims) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Dynamic Programming";
        res.algorithm = "Matrix Chain Multiplication";
        res.time_complexity = "O(n³)";
        res.space_complexity = "O(n²)";

        int n = static_cast<int>(dims.size()) - 1; // number of matrices
        if (n <= 0) return res;

        std::vector<std::vector<int>> m(n + 1, std::vector<int>(n + 1, 0));

        for (int L = 2; L <= n; ++L) {
            for (int i = 1; i <= n - L + 1; ++i) {
                int j = i + L - 1;
                m[i][j] = INT_MAX;
                for (int k = i; k <= j - 1; ++k) {
                    res.comparisons++;
                    int q = m[i][k] + m[k + 1][j] + dims[i - 1] * dims[k] * dims[j];
                    if (q < m[i][j]) {
                        m[i][j] = q;
                    }
                }
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();

        res.output_value = std::to_string(m[1][n]) + " multiplications";
        res.message = "Minimum scalar multiplications: " + std::to_string(m[1][n]);

        for (int i = 1; i <= n; ++i) res.row_headers.push_back("M" + std::to_string(i));
        for (int j = 1; j <= n; ++j) res.col_headers.push_back("M" + std::to_string(j));

        std::vector<std::vector<int>> submatrix(n, std::vector<int>(n, 0));
        for (int i = 1; i <= n; ++i) {
            for (int j = 1; j <= n; ++j) {
                submatrix[i - 1][j - 1] = m[i][j] == INT_MAX ? 0 : m[i][j];
            }
        }
        res.matrix_data = submatrix;
        return res;
    }
};

} // namespace aegis
