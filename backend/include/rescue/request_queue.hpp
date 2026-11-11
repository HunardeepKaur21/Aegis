#pragma once
#include <cstddef>
#include <stdexcept>
#include <vector>
#include "rescue/rescue_types.hpp"

namespace aegis {

// Custom FIFO Queue maintaining incoming emergency requests in order of receipt
class EmergencyRequestQueue {
private:
    struct Node {
        EmergencyRequest data;
        Node* next;
        explicit Node(const EmergencyRequest& d, Node* n = nullptr) : data(d), next(n) {}
    };

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    size_t size_ = 0;

public:
    EmergencyRequestQueue() = default;
    ~EmergencyRequestQueue() { clear(); }

    EmergencyRequestQueue(const EmergencyRequestQueue&) = delete;
    EmergencyRequestQueue& operator=(const EmergencyRequestQueue&) = delete;

    EmergencyRequestQueue(EmergencyRequestQueue&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    EmergencyRequestQueue& operator=(EmergencyRequestQueue&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = nullptr;
            other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void enqueue(const EmergencyRequest& req) {
        Node* newNode = new Node(req);
        if (tail_ == nullptr) {
            head_ = tail_ = newNode;
        } else {
            tail_->next = newNode;
            tail_ = newNode;
        }
        ++size_;
    }

    EmergencyRequest dequeue() {
        if (head_ == nullptr) {
            throw std::out_of_range("EmergencyRequestQueue::dequeue on empty queue");
        }
        Node* temp = head_;
        EmergencyRequest req = temp->data;
        head_ = head_->next;
        if (head_ == nullptr) {
            tail_ = nullptr;
        }
        delete temp;
        --size_;
        return req;
    }

    const EmergencyRequest& front() const {
        if (head_ == nullptr) {
            throw std::out_of_range("EmergencyRequestQueue::front on empty queue");
        }
        return head_->data;
    }

    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }

    void clear() {
        while (head_ != nullptr) {
            Node* temp = head_;
            head_ = head_->next;
            delete temp;
        }
        tail_ = nullptr;
        size_ = 0;
    }

    std::vector<EmergencyRequest> toList() const {
        std::vector<EmergencyRequest> result;
        result.reserve(size_);
        Node* curr = head_;
        while (curr != nullptr) {
            result.push_back(curr->data);
            curr = curr->next;
        }
        return result;
    }
};

} // namespace aegis
