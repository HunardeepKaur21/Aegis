#pragma once
#include <algorithm>
#include <cstddef>
#include <vector>

namespace aegis {

template <typename Key, typename Value>
class AVLTree {
public:
    struct Node {
        Key key;
        Value value;
        int height = 1;
        Node* left = nullptr;
        Node* right = nullptr;
        Node(Key k, Value v) : key(std::move(k)), value(std::move(v)), height(1) {}
    };

    AVLTree() = default;
    ~AVLTree() { clear(root_); }

    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    int height() const { return height(root_); }

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

    int height(Node* n) const {
        return n ? n->height : 0;
    }

    int balanceFactor(Node* n) const {
        return n ? (height(n->left) - height(n->right)) : 0;
    }

    void updateHeight(Node* n) {
        if (n) {
            n->height = 1 + std::max(height(n->left), height(n->right));
        }
    }

    // Right rotation around y (LL case)
    Node* rotateRight(Node* y) {
        Node* x = y->left;
        Node* T2 = x->right;

        x->right = y;
        y->left = T2;

        updateHeight(y);
        updateHeight(x);

        return x;
    }

    // Left rotation around x (RR case)
    Node* rotateLeft(Node* x) {
        Node* y = x->right;
        Node* T2 = y->left;

        y->left = x;
        x->right = T2;

        updateHeight(x);
        updateHeight(y);

        return y;
    }

    Node* balanceNode(Node* node) {
        updateHeight(node);
        int balance = balanceFactor(node);

        // LL Case
        if (balance > 1 && balanceFactor(node->left) >= 0) {
            return rotateRight(node);
        }

        // LR Case
        if (balance > 1 && balanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }

        // RR Case
        if (balance < -1 && balanceFactor(node->right) <= 0) {
            return rotateLeft(node);
        }

        // RL Case
        if (balance < -1 && balanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
            return rotateLeft(node);
        }

        return node;
    }

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
            node->value = std::move(value);
            return node;
        }

        return balanceNode(node);
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
            if (!node->left || !node->right) {
                Node* temp = node->left ? node->left : node->right;
                if (!temp) {
                    temp = node;
                    node = nullptr;
                } else {
                    *node = *temp;
                }
                delete temp;
            } else {
                Node* successor = findMin(node->right);
                node->key = successor->key;
                node->value = successor->value;
                node->right = removeHelper(node->right, successor->key, removed);
            }
        }

        if (!node) return nullptr;
        return balanceNode(node);
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
