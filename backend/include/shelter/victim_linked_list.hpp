#pragma once
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace aegis {

// =========================================================================
// 1. SINGLY LINKED LIST
// =========================================================================
template <typename T>
class SinglyLinkedList {
public:
    struct Node {
        T data;
        Node* next = nullptr;
        explicit Node(T val) : data(std::move(val)), next(nullptr) {}
    };

    SinglyLinkedList() = default;
    ~SinglyLinkedList() { clear(); }

    SinglyLinkedList(const SinglyLinkedList&) = delete;
    SinglyLinkedList& operator=(const SinglyLinkedList&) = delete;

    SinglyLinkedList(SinglyLinkedList&& other) noexcept : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    SinglyLinkedList& operator=(SinglyLinkedList&& other) noexcept {
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

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void insertHead(T val) {
        Node* newNode = new Node(std::move(val));
        newNode->next = head_;
        head_ = newNode;
        if (!tail_) tail_ = head_;
        ++size_;
    }

    void insertTail(T val) {
        Node* newNode = new Node(std::move(val));
        if (!tail_) {
            head_ = tail_ = newNode;
        } else {
            tail_->next = newNode;
            tail_ = newNode;
        }
        ++size_;
    }

    void insertAt(size_t index, T val) {
        if (index == 0) {
            insertHead(std::move(val));
            return;
        }
        if (index >= size_) {
            insertTail(std::move(val));
            return;
        }
        Node* curr = head_;
        for (size_t i = 0; i < index - 1; ++i) {
            curr = curr->next;
        }
        Node* newNode = new Node(std::move(val));
        newNode->next = curr->next;
        curr->next = newNode;
        ++size_;
    }

    bool deleteNode(const std::function<bool(const T&)>& predicate) {
        Node* curr = head_;
        Node* prev = nullptr;
        while (curr) {
            if (predicate(curr->data)) {
                if (prev) {
                    prev->next = curr->next;
                    if (curr == tail_) tail_ = prev;
                } else {
                    head_ = curr->next;
                    if (curr == tail_) tail_ = nullptr;
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

    Node* search(const std::function<bool(const T&)>& predicate) const {
        Node* curr = head_;
        while (curr) {
            if (predicate(curr->data)) return curr;
            curr = curr->next;
        }
        return nullptr;
    }

    void reverse() {
        Node* prev = nullptr;
        Node* curr = head_;
        tail_ = head_;
        while (curr) {
            Node* next = curr->next;
            curr->next = prev;
            prev = curr;
            curr = next;
        }
        head_ = prev;
    }

    std::vector<T> toVector() const {
        std::vector<T> vec;
        vec.reserve(size_);
        Node* curr = head_;
        while (curr) {
            vec.push_back(curr->data);
            curr = curr->next;
        }
        return vec;
    }

    void clear() {
        Node* curr = head_;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    size_t size_ = 0;
};

// =========================================================================
// 2. DOUBLY LINKED LIST
// =========================================================================
template <typename T>
class DoublyLinkedList {
public:
    struct Node {
        T data;
        Node* prev = nullptr;
        Node* next = nullptr;
        explicit Node(T val) : data(std::move(val)), prev(nullptr), next(nullptr) {}
    };

    DoublyLinkedList() = default;
    ~DoublyLinkedList() { clear(); }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void insertHead(T val) {
        Node* newNode = new Node(std::move(val));
        if (!head_) {
            head_ = tail_ = newNode;
        } else {
            newNode->next = head_;
            head_->prev = newNode;
            head_ = newNode;
        }
        ++size_;
    }

    void insertTail(T val) {
        Node* newNode = new Node(std::move(val));
        if (!tail_) {
            head_ = tail_ = newNode;
        } else {
            tail_->next = newNode;
            newNode->prev = tail_;
            tail_ = newNode;
        }
        ++size_;
    }

    bool deleteNode(const std::function<bool(const T&)>& predicate) {
        Node* curr = head_;
        while (curr) {
            if (predicate(curr->data)) {
                if (curr->prev) curr->prev->next = curr->next;
                else head_ = curr->next;

                if (curr->next) curr->next->prev = curr->prev;
                else tail_ = curr->prev;

                delete curr;
                --size_;
                return true;
            }
            curr = curr->next;
        }
        return false;
    }

    void reverse() {
        Node* curr = head_;
        Node* temp = nullptr;
        tail_ = head_;
        while (curr) {
            temp = curr->prev;
            curr->prev = curr->next;
            curr->next = temp;
            curr = curr->prev;
        }
        if (temp) head_ = temp->prev;
    }

    std::vector<T> toVector() const {
        std::vector<T> vec;
        vec.reserve(size_);
        Node* curr = head_;
        while (curr) {
            vec.push_back(curr->data);
            curr = curr->next;
        }
        return vec;
    }

    void clear() {
        Node* curr = head_;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    size_t size_ = 0;
};

} // namespace aegis
