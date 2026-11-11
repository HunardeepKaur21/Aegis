#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace aegis {

template <typename Key, typename Value>
class ChainingHashMap {
public:
    struct HashNode {
        Key key;
        Value value;
        HashNode* next = nullptr;
        HashNode(Key k, Value v) : key(std::move(k)), value(std::move(v)), next(nullptr) {}
    };

    explicit ChainingHashMap(size_t initial_buckets = 16)
        : buckets_(initial_buckets, nullptr), bucket_count_(initial_buckets), size_(0) {}

    ~ChainingHashMap() { clear(); }

    ChainingHashMap(const ChainingHashMap&) = delete;
    ChainingHashMap& operator=(const ChainingHashMap&) = delete;

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    size_t bucketCount() const { return bucket_count_; }

    void insert(Key key, Value value) {
        if (static_cast<double>(size_ + 1) / bucket_count_ > 0.75) {
            rehash(bucket_count_ * 2);
        }

        size_t idx = hashKey(key) % bucket_count_;
        HashNode* curr = buckets_[idx];

        while (curr) {
            if (curr->key == key) {
                curr->value = std::move(value);
                return;
            }
            curr = curr->next;
        }

        HashNode* newNode = new HashNode(std::move(key), std::move(value));
        newNode->next = buckets_[idx];
        buckets_[idx] = newNode;
        ++size_;
    }

    Value* find(const Key& key) {
        size_t idx = hashKey(key) % bucket_count_;
        HashNode* curr = buckets_[idx];
        while (curr) {
            if (curr->key == key) return &(curr->value);
            curr = curr->next;
        }
        return nullptr;
    }

    const Value* find(const Key& key) const {
        size_t idx = hashKey(key) % bucket_count_;
        HashNode* curr = buckets_[idx];
        while (curr) {
            if (curr->key == key) return &(curr->value);
            curr = curr->next;
        }
        return nullptr;
    }

    bool remove(const Key& key) {
        size_t idx = hashKey(key) % bucket_count_;
        HashNode* curr = buckets_[idx];
        HashNode* prev = nullptr;

        while (curr) {
            if (curr->key == key) {
                if (prev) prev->next = curr->next;
                else buckets_[idx] = curr->next;

                delete curr;
                --size_;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    std::vector<Value> allValues() const {
        std::vector<Value> result;
        result.reserve(size_);
        for (size_t i = 0; i < bucket_count_; ++i) {
            HashNode* curr = buckets_[i];
            while (curr) {
                result.push_back(curr->value);
                curr = curr->next;
            }
        }
        return result;
    }

    void clear() {
        for (size_t i = 0; i < bucket_count_; ++i) {
            HashNode* curr = buckets_[i];
            while (curr) {
                HashNode* next = curr->next;
                delete curr;
                curr = next;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

private:
    std::vector<HashNode*> buckets_;
    size_t bucket_count_ = 16;
    size_t size_ = 0;

    size_t hashKey(const Key& key) const {
        return std::hash<Key>{}(key);
    }

    void rehash(size_t new_bucket_count) {
        std::vector<HashNode*> new_buckets(new_bucket_count, nullptr);
        for (size_t i = 0; i < bucket_count_; ++i) {
            HashNode* curr = buckets_[i];
            while (curr) {
                HashNode* next = curr->next;
                size_t new_idx = hashKey(curr->key) % new_bucket_count;
                curr->next = new_buckets[new_idx];
                new_buckets[new_idx] = curr;
                curr = next;
            }
        }
        buckets_ = std::move(new_buckets);
        bucket_count_ = new_bucket_count;
    }
};

} // namespace aegis
