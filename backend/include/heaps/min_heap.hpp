#pragma once
// Custom binary min-heap (array based). Used by Dijkstra and reused by the DSA Laboratory.
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace aegis {

template <typename T, typename Less = std::less<T>>
class MinHeap {
public:
    explicit MinHeap(Less less = Less()) : less_(less) {}

    void push(const T& value) {            // O(log n)
        data_.push_back(value);
        siftUp(data_.size() - 1);
    }
    T pop() {                              // O(log n)
        if (data_.empty()) throw std::out_of_range("MinHeap::pop on empty heap");
        T smallest = data_[0];
        data_[0] = data_.back();
        data_.pop_back();
        if (!data_.empty()) siftDown(0);
        return smallest;
    }
    const T& top() const {                 // O(1)
        if (data_.empty()) throw std::out_of_range("MinHeap::top on empty heap");
        return data_[0];
    }
    bool empty() const { return data_.empty(); }
    size_t size() const { return data_.size(); }

private:
    void siftUp(size_t i) {
        while (i > 0) {
            size_t parent = (i - 1) / 2;
            if (!less_(data_[i], data_[parent])) break;
            std::swap(data_[i], data_[parent]);
            i = parent;
        }
    }
    void siftDown(size_t i) {
        const size_t n = data_.size();
        while (true) {
            size_t left = 2 * i + 1, right = left + 1, smallest = i;
            if (left < n && less_(data_[left], data_[smallest])) smallest = left;
            if (right < n && less_(data_[right], data_[smallest])) smallest = right;
            if (smallest == i) break;
            std::swap(data_[i], data_[smallest]);
            i = smallest;
        }
    }

    std::vector<T> data_;
    Less less_;
};

}  // namespace aegis
