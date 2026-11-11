#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <string>
#include <vector>
#include "lab/lab_types.hpp"

namespace aegis {

class LabSorting {
public:
    static LabRunResult run(const std::string& algo_name, const std::vector<int>& input_data) {
        std::vector<int> arr = input_data;
        std::string algo = algo_name;
        std::transform(algo.begin(), algo.end(), algo.begin(), ::tolower);

        if (algo == "bubble" || algo == "bubble sort") return bubbleSort(arr);
        if (algo == "selection" || algo == "selection sort") return selectionSort(arr);
        if (algo == "insertion" || algo == "insertion sort") return insertionSort(arr);
        if (algo == "merge" || algo == "merge sort") return mergeSort(arr);
        if (algo == "quick" || algo == "quick sort") return quickSort(arr);
        if (algo == "heap" || algo == "heap sort") return heapSort(arr);
        if (algo == "counting" || algo == "counting sort") return countingSort(arr);
        if (algo == "radix" || algo == "radix sort") return radixSort(arr);
        if (algo == "bucket" || algo == "bucket sort") return bucketSort(arr);

        return bubbleSort(arr);
    }

private:
    static void addStep(LabRunResult& res, const std::vector<int>& arr, const std::string& desc,
                        int pri = -1, int sec = -1, bool is_swap = false, bool is_comp = false) {
        if (res.steps.size() >= 200) return; // limit animation frames
        LabStep step;
        step.step_number = static_cast<int>(res.steps.size() + 1);
        step.description = desc;
        step.array_state = arr;
        step.primary_index = pri;
        step.secondary_index = sec;
        step.is_swap = is_swap;
        step.is_comparison = is_comp;
        if (pri >= 0) step.highlighted_indices.push_back(pri);
        if (sec >= 0 && sec != pri) step.highlighted_indices.push_back(sec);
        res.steps.push_back(step);
    }

    // 1. Bubble Sort
    static LabRunResult bubbleSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Bubble Sort";
        res.time_complexity = "O(n²)";
        res.space_complexity = "O(1)";

        addStep(res, arr, "Initial array state", -1, -1);
        int n = static_cast<int>(arr.size());
        for (int i = 0; i < n - 1; ++i) {
            bool swapped = false;
            for (int j = 0; j < n - i - 1; ++j) {
                res.comparisons++;
                addStep(res, arr, "Comparing arr[" + std::to_string(j) + "] and arr[" + std::to_string(j+1) + "]", j, j+1, false, true);
                if (arr[j] > arr[j + 1]) {
                    std::swap(arr[j], arr[j + 1]);
                    res.swaps++;
                    swapped = true;
                    addStep(res, arr, "Swapped " + std::to_string(arr[j+1]) + " and " + std::to_string(arr[j]), j, j+1, true, false);
                }
            }
            if (!swapped) break;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }

    // 2. Selection Sort
    static LabRunResult selectionSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Selection Sort";
        res.time_complexity = "O(n²)";
        res.space_complexity = "O(1)";

        addStep(res, arr, "Initial array state", -1, -1);
        int n = static_cast<int>(arr.size());
        for (int i = 0; i < n - 1; ++i) {
            int min_idx = i;
            for (int j = i + 1; j < n; ++j) {
                res.comparisons++;
                addStep(res, arr, "Comparing current min " + std::to_string(arr[min_idx]) + " with " + std::to_string(arr[j]), min_idx, j, false, true);
                if (arr[j] < arr[min_idx]) {
                    min_idx = j;
                }
            }
            if (min_idx != i) {
                std::swap(arr[i], arr[min_idx]);
                res.swaps++;
                addStep(res, arr, "Swapped minimum element into position " + std::to_string(i), i, min_idx, true, false);
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }

    // 3. Insertion Sort
    static LabRunResult insertionSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Insertion Sort";
        res.time_complexity = "O(n²)";
        res.space_complexity = "O(1)";

        addStep(res, arr, "Initial array state", -1, -1);
        int n = static_cast<int>(arr.size());
        for (int i = 1; i < n; ++i) {
            int key = arr[i];
            int j = i - 1;
            addStep(res, arr, "Selecting key " + std::to_string(key) + " at index " + std::to_string(i), i, -1);

            while (j >= 0) {
                res.comparisons++;
                if (arr[j] > key) {
                    arr[j + 1] = arr[j];
                    res.swaps++;
                    addStep(res, arr, "Shifted " + std::to_string(arr[j]) + " to the right", j + 1, j, true, false);
                    --j;
                } else {
                    break;
                }
            }
            arr[j + 1] = key;
            addStep(res, arr, "Inserted key " + std::to_string(key) + " at index " + std::to_string(j + 1), j + 1, -1);
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }

    // 4. Merge Sort
    static LabRunResult mergeSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Merge Sort";
        res.time_complexity = "O(n log n)";
        res.space_complexity = "O(n)";

        addStep(res, arr, "Initial array state", -1, -1);

        std::function<void(int, int)> sortHelper = [&](int left, int right) {
            if (left >= right) return;
            int mid = left + (right - left) / 2;
            sortHelper(left, mid);
            sortHelper(mid + 1, right);

            // Merge
            std::vector<int> temp;
            int i = left, j = mid + 1;
            while (i <= mid && j <= right) {
                res.comparisons++;
                if (arr[i] <= arr[j]) {
                    temp.push_back(arr[i++]);
                } else {
                    temp.push_back(arr[j++]);
                }
            }
            while (i <= mid) temp.push_back(arr[i++]);
            while (j <= right) temp.push_back(arr[j++]);

            for (size_t k = 0; k < temp.size(); ++k) {
                arr[left + k] = temp[k];
                res.swaps++;
            }
            addStep(res, arr, "Merged subarray [" + std::to_string(left) + "..." + std::to_string(right) + "]", left, right);
        };

        if (!arr.empty()) {
            sortHelper(0, static_cast<int>(arr.size() - 1));
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }

    // 5. Quick Sort
    static LabRunResult quickSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Quick Sort";
        res.time_complexity = "O(n log n)";
        res.space_complexity = "O(log n)";

        addStep(res, arr, "Initial array state", -1, -1);

        std::function<void(int, int)> qs = [&](int low, int high) {
            if (low < high) {
                int pivot = arr[high];
                int i = low - 1;
                addStep(res, arr, "Partitioning with pivot " + std::to_string(pivot) + " at index " + std::to_string(high), high, -1);

                for (int j = low; j < high; ++j) {
                    res.comparisons++;
                    if (arr[j] < pivot) {
                        ++i;
                        std::swap(arr[i], arr[j]);
                        res.swaps++;
                    }
                }
                std::swap(arr[i + 1], arr[high]);
                res.swaps++;
                int pi = i + 1;
                addStep(res, arr, "Pivot placed at index " + std::to_string(pi), pi, -1);

                qs(low, pi - 1);
                qs(pi + 1, high);
            }
        };

        if (!arr.empty()) {
            qs(0, static_cast<int>(arr.size() - 1));
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }

    // 6. Heap Sort
    static LabRunResult heapSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Heap Sort";
        res.time_complexity = "O(n log n)";
        res.space_complexity = "O(1)";

        addStep(res, arr, "Initial array state", -1, -1);
        int n = static_cast<int>(arr.size());

        std::function<void(int, int)> heapify = [&](int size, int i) {
            int largest = i;
            int left = 2 * i + 1;
            int right = 2 * i + 2;

            if (left < size) {
                res.comparisons++;
                if (arr[left] > arr[largest]) largest = left;
            }
            if (right < size) {
                res.comparisons++;
                if (arr[right] > arr[largest]) largest = right;
            }
            if (largest != i) {
                std::swap(arr[i], arr[largest]);
                res.swaps++;
                addStep(res, arr, "Heapify swap at index " + std::to_string(i), i, largest, true, false);
                heapify(size, largest);
            }
        };

        // Build heap
        for (int i = n / 2 - 1; i >= 0; --i) heapify(n, i);
        addStep(res, arr, "Max heap built", -1, -1);

        for (int i = n - 1; i > 0; --i) {
            std::swap(arr[0], arr[i]);
            res.swaps++;
            addStep(res, arr, "Extracted max element to index " + std::to_string(i), 0, i, true, false);
            heapify(i, 0);
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }

    // 7. Counting Sort
    static LabRunResult countingSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Counting Sort";
        res.time_complexity = "O(n + k)";
        res.space_complexity = "O(k)";

        addStep(res, arr, "Initial array state", -1, -1);
        if (!arr.empty()) {
            int min_val = *std::min_element(arr.begin(), arr.end());
            int max_val = *std::max_element(arr.begin(), arr.end());
            int range = max_val - min_val + 1;

            if (range <= 10000) { // feasible range
                std::vector<int> count(range, 0);
                for (int x : arr) count[x - min_val]++;
                addStep(res, arr, "Frequency counts calculated", -1, -1);

                int idx = 0;
                for (int i = 0; i < range; ++i) {
                    while (count[i] > 0) {
                        arr[idx++] = i + min_val;
                        res.swaps++;
                        count[i]--;
                    }
                }
                addStep(res, arr, "Array reconstructed from counts", -1, -1);
            } else {
                std::sort(arr.begin(), arr.end());
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }

    // 8. Radix Sort
    static LabRunResult radixSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Radix Sort";
        res.time_complexity = "O(d · (n + b))";
        res.space_complexity = "O(n + b)";

        addStep(res, arr, "Initial array state", -1, -1);
        if (!arr.empty()) {
            int max_val = std::abs(*std::max_element(arr.begin(), arr.end()));
            for (int exp = 1; max_val / exp > 0; exp *= 10) {
                std::vector<int> output(arr.size());
                int count[10] = {0};
                for (int x : arr) count[(std::abs(x) / exp) % 10]++;
                for (int i = 1; i < 10; ++i) count[i] += count[i - 1];
                for (int i = static_cast<int>(arr.size() - 1); i >= 0; --i) {
                    int digit = (std::abs(arr[i]) / exp) % 10;
                    output[count[digit] - 1] = arr[i];
                    count[digit]--;
                    res.swaps++;
                }
                arr = output;
                addStep(res, arr, "Sorted by digit position (exp=" + std::to_string(exp) + ")", -1, -1);
            }
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }

    // 9. Bucket Sort
    static LabRunResult bucketSort(std::vector<int> arr) {
        auto t0 = std::chrono::high_resolution_clock::now();
        LabRunResult res;
        res.category = "Sorting";
        res.algorithm = "Bucket Sort";
        res.time_complexity = "O(n + k)";
        res.space_complexity = "O(n)";

        addStep(res, arr, "Initial array state", -1, -1);
        if (!arr.empty()) {
            int min_val = *std::min_element(arr.begin(), arr.end());
            int max_val = *std::max_element(arr.begin(), arr.end());
            int bucket_count = std::max(1, static_cast<int>(arr.size() / 2));
            double range = std::max(1.0, static_cast<double>(max_val - min_val + 1));

            std::vector<std::vector<int>> buckets(bucket_count);
            for (int x : arr) {
                int b_idx = std::min(bucket_count - 1, static_cast<int>((x - min_val) / range * bucket_count));
                buckets[b_idx].push_back(x);
            }
            addStep(res, arr, "Distributed elements into " + std::to_string(bucket_count) + " buckets", -1, -1);

            int idx = 0;
            for (auto& b : buckets) {
                std::sort(b.begin(), b.end());
                for (int x : b) {
                    arr[idx++] = x;
                    res.swaps++;
                }
            }
            addStep(res, arr, "Concatenated sorted buckets", -1, -1);
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        res.runtime_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        res.result_array = arr;
        addStep(res, arr, "Array fully sorted", -1, -1);
        return res;
    }
};

} // namespace aegis
