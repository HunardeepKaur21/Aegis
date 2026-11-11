#pragma once
#include <cstddef>
#include <stdexcept>
#include <vector>
#include "rescue/rescue_types.hpp"

namespace aegis {

// Custom Hash Map implementation for storing and retrieving requests by integer ID in O(1) avg time.
class RequestHashMap {
private:
    struct Node {
        int key;
        EmergencyRequest value;
        Node* next;
        Node(int k, const EmergencyRequest& v, Node* n = nullptr)
            : key(k), value(v), next(n) {}
    };

    std::vector<Node*> buckets_;
    size_t size_ = 0;
    static constexpr size_t INITIAL_CAPACITY = 16;
    static constexpr double MAX_LOAD_FACTOR = 0.75;

    size_t hash(int key) const {
        // Thomas Wang's 32-bit integer hash for uniform distribution
        unsigned int k = static_cast<unsigned int>(key);
        k = ((k >> 16) ^ k) * 0x45d9f3b;
        k = ((k >> 16) ^ k) * 0x45d9f3b;
        k = (k >> 16) ^ k;
        return static_cast<size_t>(k) % buckets_.size();
    }

    void rehash() {
        size_t newCap = buckets_.size() * 2;
        std::vector<Node*> newBuckets(newCap, nullptr);

        for (size_t i = 0; i < buckets_.size(); ++i) {
            Node* curr = buckets_[i];
            while (curr != nullptr) {
                Node* next = curr->next;
                unsigned int k = static_cast<unsigned int>(curr->key);
                k = ((k >> 16) ^ k) * 0x45d9f3b;
                k = ((k >> 16) ^ k) * 0x45d9f3b;
                k = (k >> 16) ^ k;
                size_t newBucket = static_cast<size_t>(k) % newCap;

                curr->next = newBuckets[newBucket];
                newBuckets[newBucket] = curr;
                curr = next;
            }
        }
        buckets_ = std::move(newBuckets);
    }

public:
    RequestHashMap(size_t initialCapacity = INITIAL_CAPACITY)
        : buckets_(initialCapacity > 0 ? initialCapacity : INITIAL_CAPACITY, nullptr), size_(0) {}

    ~RequestHashMap() {
        clear();
    }

    // Disable copy for RAII safety, allow move
    RequestHashMap(const RequestHashMap&) = delete;
    RequestHashMap& operator=(const RequestHashMap&) = delete;

    RequestHashMap(RequestHashMap&& other) noexcept
        : buckets_(std::move(other.buckets_)), size_(other.size_) {
        other.size_ = 0;
    }

    RequestHashMap& operator=(RequestHashMap&& other) noexcept {
        if (this != &other) {
            clear();
            buckets_ = std::move(other.buckets_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    void put(int key, const EmergencyRequest& value) {
        if (buckets_.empty()) {
            buckets_.assign(INITIAL_CAPACITY, nullptr);
        }
        if (static_cast<double>(size_ + 1) / buckets_.size() > MAX_LOAD_FACTOR) {
            rehash();
        }

        size_t b = hash(key);
        Node* curr = buckets_[b];
        while (curr != nullptr) {
            if (curr->key == key) {
                curr->value = value; // update existing
                return;
            }
            curr = curr->next;
        }

        // Insert new node at bucket head
        buckets_[b] = new Node(key, value, buckets_[b]);
        ++size_;
    }

    bool has(int key) const {
        if (buckets_.empty()) return false;
        size_t b = hash(key);
        Node* curr = buckets_[b];
        while (curr != nullptr) {
            if (curr->key == key) return true;
            curr = curr->next;
        }
        return false;
    }

    EmergencyRequest* get(int key) {
        if (buckets_.empty()) return nullptr;
        size_t b = hash(key);
        Node* curr = buckets_[b];
        while (curr != nullptr) {
            if (curr->key == key) return &curr->value;
            curr = curr->next;
        }
        return nullptr;
    }

    const EmergencyRequest* get(int key) const {
        if (buckets_.empty()) return nullptr;
        size_t b = hash(key);
        Node* curr = buckets_[b];
        while (curr != nullptr) {
            if (curr->key == key) return &curr->value;
            curr = curr->next;
        }
        return nullptr;
    }

    bool remove(int key) {
        if (buckets_.empty()) return false;
        size_t b = hash(key);
        Node* curr = buckets_[b];
        Node* prev = nullptr;

        while (curr != nullptr) {
            if (curr->key == key) {
                if (prev == nullptr) {
                    buckets_[b] = curr->next;
                } else {
                    prev->next = curr->next;
                }
                delete curr;
                --size_;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear() {
        for (size_t i = 0; i < buckets_.size(); ++i) {
            Node* curr = buckets_[i];
            while (curr != nullptr) {
                Node* next = curr->next;
                delete curr;
                curr = next;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

    std::vector<EmergencyRequest> values() const {
        std::vector<EmergencyRequest> result;
        result.reserve(size_);
        for (size_t i = 0; i < buckets_.size(); ++i) {
            Node* curr = buckets_[i];
            while (curr != nullptr) {
                result.push_back(curr->value);
                curr = curr->next;
            }
        }
        return result;
    }
};

} // namespace aegis
