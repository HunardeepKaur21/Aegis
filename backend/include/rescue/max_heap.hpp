#pragma once
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace aegis {

// Custom binary max-heap with arbitrary comparator and O(log n) updatePriority by key.
template <typename T, typename KeyExtractor, typename Greater = std::greater<T>>
class MaxHeap {
public:
    explicit MaxHeap(KeyExtractor keyExtractor, Greater greater = Greater())
        : keyExtractor_(keyExtractor), greater_(greater) {}

    // O(log n) insertion
    void push(const T& value) {
        int key = keyExtractor_(value);
        size_t idx = data_.size();
        data_.push_back(value);
        keyToIndex_[key] = idx;
        siftUp(idx);
    }

    // O(log n) extract maximum
    T pop() {
        if (data_.empty()) {
            throw std::out_of_range("MaxHeap::pop on empty heap");
        }
        T maxVal = data_[0];
        int topKey = keyExtractor_(maxVal);
        keyToIndex_.erase(topKey);

        if (data_.size() == 1) {
            data_.pop_back();
            return maxVal;
        }

        data_[0] = data_.back();
        data_.pop_back();
        keyToIndex_[keyExtractor_(data_[0])] = 0;
        siftDown(0);
        return maxVal;
    }

    // O(1) peek maximum
    const T& top() const {
        if (data_.empty()) {
            throw std::out_of_range("MaxHeap::top on empty heap");
        }
        return data_[0];
    }

    bool empty() const { return data_.empty(); }
    size_t size() const { return data_.size(); }

    void clear() {
        data_.clear();
        keyToIndex_.clear();
    }

    // Returns copy of heap array in level-order
    std::vector<T> displayHeap() const {
        return data_;
    }

    // Updates priority of an element by key, and restores heap invariant
    template <typename Modifier>
    bool updatePriority(int key, Modifier modifier) {
        auto it = keyToIndex_.find(key);
        if (it == keyToIndex_.end()) {
            return false;
        }
        size_t idx = it->second;
        modifier(data_[idx]);

        // Fix heap property upwards or downwards
        size_t parent = (idx > 0) ? (idx - 1) / 2 : 0;
        if (idx > 0 && greater_(data_[idx], data_[parent])) {
            siftUp(idx);
        } else {
            siftDown(idx);
        }
        return true;
    }

    // Linear O(n) heapify from a vector
    void buildHeap(const std::vector<T>& items) {
        data_ = items;
        keyToIndex_.clear();
        for (size_t i = 0; i < data_.size(); ++i) {
            keyToIndex_[keyExtractor_(data_[i])] = i;
        }
        if (data_.size() <= 1) return;
        for (int i = static_cast<int>(data_.size() / 2) - 1; i >= 0; --i) {
            siftDown(static_cast<size_t>(i));
        }
    }

private:
    void siftUp(size_t i) {
        while (i > 0) {
            size_t parent = (i - 1) / 2;
            if (!greater_(data_[i], data_[parent])) break;
            swapNodes(i, parent);
            i = parent;
        }
    }

    void siftDown(size_t i) {
        const size_t n = data_.size();
        while (true) {
            size_t left = 2 * i + 1;
            size_t right = left + 1;
            size_t largest = i;

            if (left < n && greater_(data_[left], data_[largest])) {
                largest = left;
            }
            if (right < n && greater_(data_[right], data_[largest])) {
                largest = right;
            }
            if (largest == i) break;
            swapNodes(i, largest);
            i = largest;
        }
    }

    void swapNodes(size_t a, size_t b) {
        std::swap(data_[a], data_[b]);
        keyToIndex_[keyExtractor_(data_[a])] = a;
        keyToIndex_[keyExtractor_(data_[b])] = b;
    }

    std::vector<T> data_;
    std::unordered_map<int, size_t> keyToIndex_;
    KeyExtractor keyExtractor_;
    Greater greater_;
};

} // namespace aegis
