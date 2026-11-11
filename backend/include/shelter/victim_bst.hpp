#pragma once
#include <cstddef>
#include <functional>
#include <vector>

namespace aegis {

template <typename Key, typename Value>
class BinarySearchTree {
public:
    struct Node {
        Key key;
        Value value;
        Node* left = nullptr;
        Node* right = nullptr;
        Node(Key k, Value v) : key(std::move(k)), value(std::move(v)) {}
    };

    BinarySearchTree() = default;
    ~BinarySearchTree() { clear(root_); }

    BinarySearchTree(const BinarySearchTree&) = delete;
    BinarySearchTree& operator=(const BinarySearchTree&) = delete;

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void insert(Key key, Value value) {
        root_ = insertHelper(root_, std::move(key), std::move(value));
    }

    Value* search(const Key& key) {
        Node* curr = root_;
        while (curr) {
            if (key == curr->key) return &(curr->value);
            if (key < curr->key) curr = curr->left;
            else curr = curr->right;
        }
        return nullptr;
    }

    const Value* search(const Key& key) const {
        Node* curr = root_;
        while (curr) {
            if (key == curr->key) return &(curr->value);
            if (key < curr->key) curr = curr->left;
            else curr = curr->right;
        }
        return nullptr;
    }

    bool remove(const Key& key) {
        bool removed = false;
        root_ = removeHelper(root_, key, removed);
        if (removed) --size_;
        return removed;
    }

    std::vector<Value> inorderTraversal() const {
        std::vector<Value> result;
        result.reserve(size_);
        inorderHelper(root_, result);
        return result;
    }

    void clear() {
        clear(root_);
        root_ = nullptr;
        size_ = 0;
    }

private:
    Node* root_ = nullptr;
    size_t size_ = 0;

    Node* insertHelper(Node* node, Key key, Value value) {
        if (!node) {
            ++size_;
            return new Node(std::move(key), std::move(value));
        }
        if (key < node->key) {
            node->left = insertHelper(node->left, std::move(key), std::move(value));
        } else if (key > node->key) {
            node->right = insertHelper(node->right, std::move(key), std::move(value));
        } else {
            node->value = std::move(value); // Update if key exists
        }
        return node;
    }

    Node* findMin(Node* node) const {
        while (node && node->left) node = node->left;
        return node;
    }

    Node* removeHelper(Node* node, const Key& key, bool& removed) {
        if (!node) return nullptr;

        if (key < node->key) {
            node->left = removeHelper(node->left, key, removed);
        } else if (key > node->key) {
            node->right = removeHelper(node->right, key, removed);
        } else {
            removed = true;
            if (!node->left) {
                Node* rightChild = node->right;
                delete node;
                return rightChild;
            } else if (!node->right) {
                Node* leftChild = node->left;
                delete node;
                return leftChild;
            } else {
                Node* successor = findMin(node->right);
                node->key = successor->key;
                node->value = successor->value;
                node->right = removeHelper(node->right, successor->key, removed);
            }
        }
        return node;
    }

    void inorderHelper(Node* node, std::vector<Value>& out) const {
        if (!node) return;
        inorderHelper(node->left, out);
        out.push_back(node->value);
        inorderHelper(node->right, out);
    }

    void clear(Node* node) {
        if (!node) return;
        clear(node->left);
        clear(node->right);
        delete node;
    }
};

} // namespace aegis
