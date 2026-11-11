#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <map>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include "lab/lab_types.hpp"

namespace aegis {

class LabStructures {
public:
    // =========================================================
    // 1. ARRAYS
    // =========================================================
    static LabRunResult runArray(const std::vector<int>& arr, const std::string& op, int index, int value) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Arrays";
        res.algorithm = "Dynamic Array";
        res.operation = op;
        std::vector<int> working = arr;

        if (op == "insert") {
            res.time_complexity = "O(n)";
            res.space_complexity = "O(1)";
            if (index < 0) index = 0;
            if (index > static_cast<int>(working.size())) index = static_cast<int>(working.size());
            working.insert(working.begin() + index, value);
            res.message = "Inserted " + std::to_string(value) + " at index " + std::to_string(index);
        } else if (op == "delete") {
            res.time_complexity = "O(n)";
            res.space_complexity = "O(1)";
            if (index >= 0 && index < static_cast<int>(working.size())) {
                int removed = working[index];
                working.erase(working.begin() + index);
                res.message = "Deleted " + std::to_string(removed) + " at index " + std::to_string(index);
            } else {
                res.message = "Index out of bounds";
            }
        } else if (op == "update") {
            res.time_complexity = "O(1)";
            res.space_complexity = "O(1)";
            if (index >= 0 && index < static_cast<int>(working.size())) {
                working[index] = value;
                res.message = "Updated index " + std::to_string(index) + " to " + std::to_string(value);
            }
        } else { // traverse
            res.time_complexity = "O(n)";
            res.space_complexity = "O(1)";
            res.message = "Traversed " + std::to_string(working.size()) + " elements";
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = working;
        return res;
    }

    // =========================================================
    // 2. STRINGS (KMP, Anagram, Palindrome, Reverse, Freq)
    // =========================================================
    static LabRunResult runString(const std::string& text, const std::string& pattern, const std::string& op) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Strings";
        res.operation = op;

        if (op == "reverse") {
            res.algorithm = "String Reversal";
            res.time_complexity = "O(n)";
            res.space_complexity = "O(n)";
            std::string rev = text;
            std::reverse(rev.begin(), rev.end());
            res.output_value = rev;
            res.message = "Reversed string: '" + rev + "'";
        } else if (op == "palindrome") {
            res.algorithm = "Palindrome Check";
            res.time_complexity = "O(n)";
            res.space_complexity = "O(1)";
            std::string rev = text;
            std::reverse(rev.begin(), rev.end());
            bool is_pal = (text == rev);
            res.output_value = is_pal ? "TRUE (Is Palindrome)" : "FALSE (Not Palindrome)";
            res.message = is_pal ? "'" + text + "' reads identically backwards" : "'" + text + "' is not a palindrome";
        } else if (op == "anagram") {
            res.algorithm = "Anagram Check";
            res.time_complexity = "O(n)";
            res.space_complexity = "O(1)";
            std::string t1 = text, t2 = pattern;
            std::sort(t1.begin(), t1.end());
            std::sort(t2.begin(), t2.end());
            bool is_ana = (t1 == t2);
            res.output_value = is_ana ? "TRUE (Anagrams)" : "FALSE (Not Anagrams)";
            res.message = is_ana ? "'" + text + "' and '" + pattern + "' are valid anagrams" : "Strings have different character distributions";
        } else if (op == "frequency") {
            res.algorithm = "Character Frequency";
            res.time_complexity = "O(n)";
            res.space_complexity = "O(k)";
            std::map<char, int> freq;
            for (char c : text) freq[c]++;
            for (auto const& [ch, cnt] : freq) {
                res.key_value_pairs.push_back({std::string(1, ch), std::to_string(cnt)});
            }
            res.output_value = std::to_string(freq.size()) + " unique characters";
            res.message = "Calculated frequency distribution across " + std::to_string(text.size()) + " characters";
        } else if (op == "kmp" || op == "pattern") {
            res.algorithm = "KMP Pattern Matching";
            res.time_complexity = "O(n + m)";
            res.space_complexity = "O(m)";

            // Preprocess LPS array
            int m = static_cast<int>(pattern.size());
            int n = static_cast<int>(text.size());
            std::vector<int> lps(m, 0);
            int len = 0, i = 1;
            while (i < m) {
                if (pattern[i] == pattern[len]) {
                    lps[i++] = ++len;
                } else if (len != 0) {
                    len = lps[len - 1];
                } else {
                    lps[i++] = 0;
                }
            }

            // KMP Search
            std::vector<int> matches;
            i = 0; int j = 0;
            while (i < n) {
                res.comparisons++;
                if (pattern[j] == text[i]) {
                    i++; j++;
                }
                if (j == m) {
                    matches.push_back(i - j);
                    j = lps[j - 1];
                } else if (i < n && pattern[j] != text[i]) {
                    if (j != 0) j = lps[j - 1];
                    else i++;
                }
            }

            res.result_array = matches;
            if (matches.empty()) {
                res.output_value = "No Match Found";
                res.message = "Pattern '" + pattern + "' not found in text";
            } else {
                std::ostringstream ss;
                ss << "Found " << matches.size() << " match(es) at index(es): ";
                for (size_t k = 0; k < matches.size(); ++k) {
                    if (k > 0) ss << ", ";
                    ss << matches[k];
                }
                res.output_value = ss.str();
                res.message = ss.str();
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        return res;
    }

    // =========================================================
    // 3. STACK (Push, Pop, Peek)
    // =========================================================
    static LabRunResult runStack(const std::vector<int>& items, const std::string& op, int value) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Stacks";
        res.algorithm = "LIFO Stack";
        res.operation = op;
        res.time_complexity = "O(1)";
        res.space_complexity = "O(n)";
        std::vector<int> working = items;

        if (op == "push") {
            if (working.size() >= 20) {
                res.success = false;
                res.message = "Stack Overflow! Maximum educational limit reached.";
            } else {
                working.push_back(value);
                res.output_value = std::to_string(value);
                res.message = "Pushed " + std::to_string(value) + " onto top of stack";
            }
        } else if (op == "pop") {
            if (working.empty()) {
                res.success = false;
                res.message = "Stack Underflow! Cannot pop from empty stack.";
            } else {
                int popped = working.back();
                working.pop_back();
                res.output_value = std::to_string(popped);
                res.message = "Popped " + std::to_string(popped) + " from top of stack";
            }
        } else { // peek
            if (working.empty()) {
                res.output_value = "Empty";
                res.message = "Stack is currently empty";
            } else {
                res.output_value = std::to_string(working.back());
                res.message = "Top element is " + std::to_string(working.back());
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = working;
        return res;
    }

    // =========================================================
    // 4. QUEUE (Enqueue, Dequeue, Front, Rear)
    // =========================================================
    static LabRunResult runQueue(const std::vector<int>& items, const std::string& op, int value) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Queues";
        res.algorithm = "FIFO Queue";
        res.operation = op;
        res.time_complexity = "O(1)";
        res.space_complexity = "O(n)";
        std::vector<int> working = items;

        if (op == "enqueue") {
            if (working.size() >= 20) {
                res.success = false;
                res.message = "Queue Overflow! Maximum capacity reached.";
            } else {
                working.push_back(value);
                res.output_value = std::to_string(value);
                res.message = "Enqueued " + std::to_string(value) + " at rear";
            }
        } else if (op == "dequeue") {
            if (working.empty()) {
                res.success = false;
                res.message = "Queue Underflow! Cannot dequeue from empty queue.";
            } else {
                int front = working.front();
                working.erase(working.begin());
                res.output_value = std::to_string(front);
                res.message = "Dequeued " + std::to_string(front) + " from front";
            }
        } else { // peek
            if (working.empty()) {
                res.output_value = "Empty";
                res.message = "Queue is currently empty";
            } else {
                res.output_value = "Front: " + std::to_string(working.front()) + ", Rear: " + std::to_string(working.back());
                res.message = "Front is " + std::to_string(working.front()) + "; Rear is " + std::to_string(working.back());
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = working;
        return res;
    }

    // =========================================================
    // 5. TREES (Binary Tree Traversals, Height, Node Count)
    // =========================================================
    static LabRunResult runTree(const std::vector<int>& nodes, const std::string& traversal) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Trees";
        res.algorithm = "Binary Tree";
        res.operation = traversal;
        res.time_complexity = "O(n)";
        res.space_complexity = "O(h)";

        // Build simple binary tree from level order array
        struct TNode {
            int val;
            TNode* left = nullptr;
            TNode* right = nullptr;
            explicit TNode(int v) : val(v) {}
        };

        if (nodes.empty()) {
            res.output_value = "Empty Tree";
            res.message = "Tree is empty";
            return res;
        }

        TNode* root = new TNode(nodes[0]);
        std::queue<TNode*> q;
        q.push(root);
        size_t idx = 1;

        while (!q.empty() && idx < nodes.size()) {
            TNode* curr = q.front();
            q.pop();

            if (idx < nodes.size()) {
                curr->left = new TNode(nodes[idx++]);
                q.push(curr->left);
            }
            if (idx < nodes.size()) {
                curr->right = new TNode(nodes[idx++]);
                q.push(curr->right);
            }
        }

        std::vector<int> result;

        std::function<void(TNode*)> pre = [&](TNode* n) {
            if (!n) return;
            result.push_back(n->val);
            pre(n->left);
            pre(n->right);
        };
        std::function<void(TNode*)> in = [&](TNode* n) {
            if (!n) return;
            in(n->left);
            result.push_back(n->val);
            in(n->right);
        };
        std::function<void(TNode*)> post = [&](TNode* n) {
            if (!n) return;
            post(n->left);
            post(n->right);
            result.push_back(n->val);
        };
        std::function<int(TNode*)> height = [&](TNode* n) -> int {
            if (!n) return 0;
            return 1 + std::max(height(n->left), height(n->right));
        };

        if (traversal == "preorder") pre(root);
        else if (traversal == "inorder") in(root);
        else if (traversal == "postorder") post(root);
        else result = nodes; // level-order

        int h = height(root);
        res.result_array = result;
        res.output_value = "Height: " + std::to_string(h) + ", Nodes: " + std::to_string(nodes.size());

        std::ostringstream ss;
        ss << traversal << " traversal: [";
        for (size_t i = 0; i < result.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << result[i];
            res.traversal_order.push_back(std::to_string(result[i]));
        }
        ss << "]";
        res.message = ss.str();

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        return res;
    }

    // =========================================================
    // 6. HASHING (Chaining & Probing)
    // =========================================================
    static LabRunResult runHashing(const std::vector<int>& keys, const std::string& method, int table_size = 10) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Hashing";
        res.algorithm = method;
        res.time_complexity = "O(1) average";
        res.space_complexity = "O(m)";

        if (table_size <= 0) table_size = 10;
        if (table_size > 50) table_size = 50;

        if (method == "chaining" || method == "Separate Chaining") {
            std::vector<std::vector<int>> table(table_size);
            for (int k : keys) {
                int idx = std::abs(k) % table_size;
                table[idx].push_back(k);
            }
            for (int i = 0; i < table_size; ++i) {
                std::ostringstream ss;
                if (table[i].empty()) {
                    ss << "Empty";
                } else {
                    for (size_t j = 0; j < table[i].size(); ++j) {
                        if (j > 0) ss << " -> ";
                        ss << table[i][j];
                    }
                }
                res.key_value_pairs.push_back({"Slot [" + std::to_string(i) + "]", ss.str()});
            }
            res.message = "Separate Chaining across " + std::to_string(table_size) + " buckets";
        } else { // Linear Probing
            std::vector<int> table(table_size, -1);
            long long col = 0;
            for (int k : keys) {
                int idx = std::abs(k) % table_size;
                int original = idx;
                while (table[idx] != -1) {
                    col++;
                    idx = (idx + 1) % table_size;
                    if (idx == original) break; // full
                }
                if (table[idx] == -1) table[idx] = k;
            }
            res.comparisons = col;
            for (int i = 0; i < table_size; ++i) {
                res.key_value_pairs.push_back({"Slot [" + std::to_string(i) + "]", table[i] == -1 ? "Empty" : std::to_string(table[i])});
            }
            res.message = "Linear Probing resolved with " + std::to_string(col) + " collisions";
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.output_value = std::to_string(keys.size()) + " keys mapped";
        return res;
    }

    // =========================================================
    // 7. HEAPS (Min & Max Heap, Heapify)
    // =========================================================
    static LabRunResult runHeap(const std::vector<int>& input, bool is_max) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Heaps";
        res.algorithm = is_max ? "Max Heap" : "Min Heap";
        res.time_complexity = "O(n) build, O(log n) op";
        res.space_complexity = "O(1)";

        std::vector<int> heap = input;
        int n = static_cast<int>(heap.size());

        auto heapify = [&](auto& self, int sz, int i) -> void {
            int target = i;
            int l = 2 * i + 1;
            int r = 2 * i + 2;

            if (is_max) {
                if (l < sz && heap[l] > heap[target]) target = l;
                if (r < sz && heap[r] > heap[target]) target = r;
            } else {
                if (l < sz && heap[l] < heap[target]) target = l;
                if (r < sz && heap[r] < heap[target]) target = r;
            }
            if (target != i) {
                std::swap(heap[i], heap[target]);
                res.swaps++;
                self(self, sz, target);
            }
        };

        for (int i = n / 2 - 1; i >= 0; --i) {
            heapify(heapify, n, i);
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = heap;
        res.output_value = heap.empty() ? "Empty" : "Root: " + std::to_string(heap[0]);
        res.message = (is_max ? "Max Heap" : "Min Heap") + std::string(" built with ") + std::to_string(res.swaps) + " heapify swaps";
        return res;
    }
};

} // namespace aegis
