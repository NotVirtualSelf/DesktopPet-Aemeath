#include "AVLTree.h"

// ==================== 节点定义 ====================
template <typename T, typename Compare, typename Allocator>
struct AVLTree<T, Compare, Allocator>::Node {
    T data;
    Node* left;
    Node* right;
    Node* parent;
    int height;
    int size;

    explicit Node(const T& val)
        : data(val), left(nullptr), right(nullptr), parent(nullptr), height(1), size(1) {}
};

// ==================== 构造/析构 ====================
template <typename T, typename Compare, typename Allocator>
AVLTree<T, Compare, Allocator>::AVLTree()
    : root(nullptr), comp(Compare()), node_alloc(typename std::allocator_traits<Allocator>::template rebind_alloc<Node>()) {}

template <typename T, typename Compare, typename Allocator>
AVLTree<T, Compare, Allocator>::AVLTree(const Compare& comp, const Allocator& alloc)
    : root(nullptr), comp(comp), node_alloc(alloc) {}

template <typename T, typename Compare, typename Allocator>
AVLTree<T, Compare, Allocator>::~AVLTree() {
    clear();
}

template <typename T, typename Compare, typename Allocator>
AVLTree<T, Compare, Allocator>::AVLTree(AVLTree&& other) noexcept
    : root(other.root), comp(std::move(other.comp)), node_alloc(std::move(other.node_alloc)) {
    other.root = nullptr;
}

template <typename T, typename Compare, typename Allocator>
AVLTree<T, Compare, Allocator>& AVLTree<T, Compare, Allocator>::operator=(AVLTree&& other) noexcept {
    if (this != &other) {
        clear();
        root = other.root;
        comp = std::move(other.comp);
        node_alloc = std::move(other.node_alloc);
        other.root = nullptr;
    }
    return *this;
}

// ==================== 容量 ====================
template <typename T, typename Compare, typename Allocator>
bool AVLTree<T, Compare, Allocator>::empty() const {
    return root == nullptr;
}

template <typename T, typename Compare, typename Allocator>
int AVLTree<T, Compare, Allocator>::size() const {
    return getSize(root);
}

template <typename T, typename Compare, typename Allocator>
int AVLTree<T, Compare, Allocator>::height() const {
    return getHeight(root);
}

// ==================== 节点辅助 ====================
template <typename T, typename Compare, typename Allocator>
int AVLTree<T, Compare, Allocator>::getHeight(Node* node) const {
    return node ? node->height : 0;
}

template <typename T, typename Compare, typename Allocator>
int AVLTree<T, Compare, Allocator>::getSize(Node* node) const {
    return node ? node->size : 0;
}

template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::updateHeight(Node* node) {
    if (node) {
        node->height = 1 + std::max(getHeight(node->left), getHeight(node->right));
    }
}

template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::updateSize(Node* node) {
    if (node) {
        node->size = 1 + getSize(node->left) + getSize(node->right);
    }
}

template <typename T, typename Compare, typename Allocator>
int AVLTree<T, Compare, Allocator>::getBalance(Node* node) const {
    return node ? getHeight(node->left) - getHeight(node->right) : 0;
}

// ==================== 节点内存管理 ====================
template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::createNode(const T& value) {
    Node* p = NodeAllocTraits::allocate(node_alloc, 1);
    try {
        NodeAllocTraits::construct(node_alloc, p, value);
    } catch (...) {
        NodeAllocTraits::deallocate(node_alloc, p, 1);
        throw;
    }
    return p;
}

template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::destroyNode(Node* node) {
    if (node) {
        NodeAllocTraits::destroy(node_alloc, node);
        NodeAllocTraits::deallocate(node_alloc, node, 1);
    }
}

// ==================== 旋转 ====================
template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::rotateRight(Node* y) {
    Node* x = y->left;
    Node* T2 = x->right;

    x->right = y;
    y->left = T2;

    x->parent = y->parent;
    y->parent = x;
    if (T2) T2->parent = y;

    updateHeight(y);
    updateHeight(x);
    updateSize(y);
    updateSize(x);
    return x;
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::rotateLeft(Node* x) {
    Node* y = x->right;
    Node* T2 = y->left;

    y->left = x;
    x->right = T2;

    y->parent = x->parent;
    x->parent = y;
    if (T2) T2->parent = x;

    updateHeight(x);
    updateHeight(y);
    updateSize(x);
    updateSize(y);
    return y;
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::balance(Node* node) {
    if (!node) return node;
    int bf = getBalance(node);

    // Left Left
    if (bf > 1 && getBalance(node->left) >= 0)
        return rotateRight(node);
    // Left Right
    if (bf > 1 && getBalance(node->left) < 0) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    // Right Right
    if (bf < -1 && getBalance(node->right) <= 0)
        return rotateLeft(node);
    // Right Left
    if (bf < -1 && getBalance(node->right) > 0) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }
    return node;
}

// ==================== 插入 ====================
template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::insert(const T& value) {
    root = insertRec(root, value);
    if (root) root->parent = nullptr;
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::insertRec(Node* node, const T& value) {
    if (!node) return createNode(value);

    if (comp(value, node->data)) {
        node->left = insertRec(node->left, value);
        if (node->left) node->left->parent = node;
    } else if (comp(node->data, value)) {
        node->right = insertRec(node->right, value);
        if (node->right) node->right->parent = node;
    } else {
        return node; // 重复，忽略
    }

    updateHeight(node);
    updateSize(node);
    return balance(node);
}

// ==================== 查找 ====================
template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::findNode(const T& value) const {
    Node* cur = root;
    while (cur) {
        if (comp(value, cur->data))
            cur = cur->left;
        else if (comp(cur->data, value))
            cur = cur->right;
        else
            return cur;
    }
    return nullptr;
}

template <typename T, typename Compare, typename Allocator>
bool AVLTree<T, Compare, Allocator>::contains(const T& value) const {
    return findNode(value) != nullptr;
}

// ==================== 删除 ====================
template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::findMin(Node* node) const {
    if (!node) return nullptr;
    while (node->left) node = node->left;
    return node;
}

template <typename T, typename Compare, typename Allocator>
bool AVLTree<T, Compare, Allocator>::erase(const T& value) {
    if (!root) return false;
    root = eraseRec(root, value);
    if (root) root->parent = nullptr;
    return true;
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::eraseRec(Node* node, const T& value) {
    if (!node) return nullptr;

    if (comp(value, node->data)) {
        node->left = eraseRec(node->left, value);
        if (node->left) node->left->parent = node;
    } else if (comp(node->data, value)) {
        node->right = eraseRec(node->right, value);
        if (node->right) node->right->parent = node;
    } else {
        if (!node->left || !node->right) {
            Node* temp = node->left ? node->left : node->right;
            if (!temp) {
                destroyNode(node);
                return nullptr;
            } else {
                Node* child = temp;
                node->data = std::move(child->data);
                node->left = child->left;
                node->right = child->right;
                node->height = child->height;
                node->size = child->size;
                if (node->left) node->left->parent = node;
                if (node->right) node->right->parent = node;
                destroyNode(child);
            }
        } else {
            Node* minRight = findMin(node->right);
            node->data = minRight->data;
            node->right = eraseRec(node->right, minRight->data);
            if (node->right) node->right->parent = node;
        }
    }

    updateHeight(node);
    updateSize(node);
    return balance(node);
}

// ==================== 清空 ====================
template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::clear() {
    clearRec(root);
    root = nullptr;
}

template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::clearRec(Node* node) {
    if (node) {
        clearRec(node->left);
        clearRec(node->right);
        destroyNode(node);
    }
}

// ==================== 排名 ====================
template <typename T, typename Compare, typename Allocator>
int AVLTree<T, Compare, Allocator>::rank(const T& value) const {
    return rankRec(root, value);
}

template <typename T, typename Compare, typename Allocator>
int AVLTree<T, Compare, Allocator>::rankRec(Node* node, const T& value) const {
    if (!node) return 0;
    if (comp(value, node->data))
        return rankRec(node->left, value);
    else if (comp(node->data, value)) {
        int leftSize = getSize(node->left);
        int rightRank = rankRec(node->right, value);
        return leftSize + 1 + rightRank;
    } else {
        return getSize(node->left);
    }
}

// ==================== 第k小 ====================
template <typename T, typename Compare, typename Allocator>
T& AVLTree<T, Compare, Allocator>::kth(int k) {
    if (k < 0 || k >= size()) throw std::out_of_range("kth index out of range");
    return kthRec(root, k)->data;
}

template <typename T, typename Compare, typename Allocator>
const T& AVLTree<T, Compare, Allocator>::kth(int k) const {
    if (k < 0 || k >= size()) throw std::out_of_range("kth index out of range");
    return kthRec(root, k)->data;
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::kthRec(Node* node, int k) const {
    int leftSize = getSize(node->left);
    if (k < leftSize)
        return kthRec(node->left, k);
    else if (k == leftSize)
        return node;
    else
        return kthRec(node->right, k - leftSize - 1);
}

// ==================== 前驱/后继 ====================
template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::iterator
AVLTree<T, Compare, Allocator>::predecessor(const T& value) {
    Node* pred = nullptr;
    Node* cur = root;
    while (cur) {
        if (comp(cur->data, value)) {
            pred = cur;
            cur = cur->right;
        } else {
            cur = cur->left;
        }
    }
    return iterator(pred);
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_iterator
AVLTree<T, Compare, Allocator>::predecessor(const T& value) const {
    Node* pred = nullptr;
    Node* cur = root;
    while (cur) {
        if (comp(cur->data, value)) {
            pred = cur;
            cur = cur->right;
        } else {
            cur = cur->left;
        }
    }
    return const_iterator(pred);
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::iterator
AVLTree<T, Compare, Allocator>::successor(const T& value) {
    Node* succ = nullptr;
    Node* cur = root;
    while (cur) {
        if (comp(value, cur->data)) {
            succ = cur;
            cur = cur->left;
        } else {
            cur = cur->right;
        }
    }
    return iterator(succ);
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_iterator
AVLTree<T, Compare, Allocator>::successor(const T& value) const {
    Node* succ = nullptr;
    Node* cur = root;
    while (cur) {
        if (comp(value, cur->data)) {
            succ = cur;
            cur = cur->left;
        } else {
            cur = cur->right;
        }
    }
    return const_iterator(succ);
}

// ==================== 范围查询 ====================
template <typename T, typename Compare, typename Allocator>
std::vector<T> AVLTree<T, Compare, Allocator>::range_query(const T& low, const T& high) const {
    std::vector<T> result;
    range_query_rec(root, low, high, result);
    return result;
}

template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::range_query_rec(Node* node, const T& low, const T& high, std::vector<T>& out) const {
    if (!node) return;
    if (!comp(node->data, low))   // node->data >= low
        range_query_rec(node->left, low, high, out);
    if (!comp(node->data, low) && !comp(high, node->data)) // low <= node->data <= high
        out.push_back(node->data);
    if (!comp(high, node->data))  // node->data <= high
        range_query_rec(node->right, low, high, out);
}

// ==================== 范围删除 ====================
template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::range_erase(const T& low, const T& high) {
    root = range_erase_rec(root, low, high);
    if (root) root->parent = nullptr;
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTree<T, Compare, Allocator>::range_erase_rec(Node* node, const T& low, const T& high) {
    if (!node) return nullptr;

    if (comp(node->data, low)) {
        node->right = range_erase_rec(node->right, low, high);
        if (node->right) node->right->parent = node;
        updateHeight(node);
        updateSize(node);
        return balance(node);
    }
    if (comp(high, node->data)) {
        node->left = range_erase_rec(node->left, low, high);
        if (node->left) node->left->parent = node;
        updateHeight(node);
        updateSize(node);
        return balance(node);
    }

    // 当前节点在区间内，需要删除
    Node* new_subroot = eraseRec(node, node->data);
    new_subroot = range_erase_rec(new_subroot, low, high);
    return new_subroot;
}

// ==================== 迭代器接口 ====================
template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::iterator
AVLTree<T, Compare, Allocator>::begin() {
    return iterator(findMin(root));
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::iterator
AVLTree<T, Compare, Allocator>::end() {
    return iterator(nullptr);
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_iterator
AVLTree<T, Compare, Allocator>::begin() const {
    return const_iterator(findMin(root));
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_iterator
AVLTree<T, Compare, Allocator>::end() const {
    return const_iterator(nullptr);
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_iterator
AVLTree<T, Compare, Allocator>::cbegin() const {
    return begin();
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_iterator
AVLTree<T, Compare, Allocator>::cend() const {
    return end();
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::reverse_iterator
AVLTree<T, Compare, Allocator>::rbegin() {
    return reverse_iterator(end());
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::reverse_iterator
AVLTree<T, Compare, Allocator>::rend() {
    return reverse_iterator(begin());
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_reverse_iterator
AVLTree<T, Compare, Allocator>::rbegin() const {
    return const_reverse_iterator(end());
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_reverse_iterator
AVLTree<T, Compare, Allocator>::rend() const {
    return const_reverse_iterator(begin());
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_reverse_iterator
AVLTree<T, Compare, Allocator>::crbegin() const {
    return rbegin();
}

template <typename T, typename Compare, typename Allocator>
typename AVLTree<T, Compare, Allocator>::const_reverse_iterator
AVLTree<T, Compare, Allocator>::crend() const {
    return rend();
}

// ==================== 调试输出 ====================
template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::inorder() const {
    inorderRec(root);
    std::cout << std::endl;
}

template <typename T, typename Compare, typename Allocator>
void AVLTree<T, Compare, Allocator>::inorderRec(Node* node) const {
    if (node) {
        inorderRec(node->left);
        std::cout << node->data << " ";
        inorderRec(node->right);
    }
}

// ==================== 迭代器成员定义 ====================
template <typename T, typename Compare, typename Allocator, bool IsConst>
AVLTreeIterator<T, Compare, Allocator, IsConst>::AVLTreeIterator(typename AVLTree<T, Compare, Allocator>::Node* node)
    : ptr(node) {}

template <typename T, typename Compare, typename Allocator, bool IsConst>
template <bool OtherConst, typename>
AVLTreeIterator<T, Compare, Allocator, IsConst>::AVLTreeIterator(const AVLTreeIterator<T, Compare, Allocator, OtherConst>& other)
    : ptr(other.ptr) {}

template <typename T, typename Compare, typename Allocator, bool IsConst>
typename AVLTreeIterator<T, Compare, Allocator, IsConst>::reference
AVLTreeIterator<T, Compare, Allocator, IsConst>::operator*() const {
    return ptr->data;
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
typename AVLTreeIterator<T, Compare, Allocator, IsConst>::pointer
AVLTreeIterator<T, Compare, Allocator, IsConst>::operator->() const {
    return &ptr->data;
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
void AVLTreeIterator<T, Compare, Allocator, IsConst>::increment() {
    if (!ptr) return;
    if (ptr->right) {
        ptr = ptr->right;
        while (ptr->left) ptr = ptr->left;
    } else {
        auto child = ptr;
        auto parent = ptr->parent;
        while (parent && parent->right == child) {
            child = parent;
            parent = parent->parent;
        }
        ptr = parent;
    }
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
void AVLTreeIterator<T, Compare, Allocator, IsConst>::decrement() {
    if (!ptr) {
        // 对于反向迭代器适配的情况，递减 nullptr 是不允许的，但这里简化处理
        return;
    }
    if (ptr->left) {
        ptr = ptr->left;
        while (ptr->right) ptr = ptr->right;
    } else {
        auto child = ptr;
        auto parent = ptr->parent;
        while (parent && parent->left == child) {
            child = parent;
            parent = parent->parent;
        }
        ptr = parent;
    }
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
AVLTreeIterator<T, Compare, Allocator, IsConst>&
AVLTreeIterator<T, Compare, Allocator, IsConst>::operator++() {
    increment();
    return *this;
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
AVLTreeIterator<T, Compare, Allocator, IsConst>
AVLTreeIterator<T, Compare, Allocator, IsConst>::operator++(int) {
    auto tmp = *this;
    increment();
    return tmp;
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
AVLTreeIterator<T, Compare, Allocator, IsConst>&
AVLTreeIterator<T, Compare, Allocator, IsConst>::operator--() {
    decrement();
    return *this;
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
AVLTreeIterator<T, Compare, Allocator, IsConst>
AVLTreeIterator<T, Compare, Allocator, IsConst>::operator--(int) {
    auto tmp = *this;
    decrement();
    return tmp;
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
bool AVLTreeIterator<T, Compare, Allocator, IsConst>::operator==(const AVLTreeIterator& other) const {
    return ptr == other.ptr;
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
bool AVLTreeIterator<T, Compare, Allocator, IsConst>::operator!=(const AVLTreeIterator& other) const {
    return ptr != other.ptr;
}

template <typename T, typename Compare, typename Allocator, bool IsConst>
typename AVLTree<T, Compare, Allocator>::Node*
AVLTreeIterator<T, Compare, Allocator, IsConst>::base() const {
    return ptr;
}