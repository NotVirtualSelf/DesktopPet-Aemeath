#ifndef AVLTREE_H
#define AVLTREE_H

#include <functional>
#include <memory>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <iostream>

// 前向声明迭代器类（需声明为模板，以便在类中作为友元）
template <typename T, typename Compare, typename Allocator, bool IsConst>
class AVLTreeIterator;

// AVL树模板声明
template <typename T,
          typename Compare = std::less<T>,
          typename Allocator = std::allocator<T>>
class AVLTree {
public:
    // 类型定义
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare = Compare;
    using allocator_type = Allocator;

    // 迭代器类型定义
    using iterator = AVLTreeIterator<T, Compare, Allocator, false>;
    using const_iterator = AVLTreeIterator<T, Compare, Allocator, true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // 构造/析构
    AVLTree();
    explicit AVLTree(const Compare& comp, const Allocator& alloc = Allocator());
    ~AVLTree();

    // 移动语义
    AVLTree(AVLTree&& other) noexcept;
    AVLTree& operator=(AVLTree&& other) noexcept;

    // 禁止拷贝
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    // 容量
    bool empty() const;
    int size() const;
    int height() const;

    // 修改
    void insert(const T& value);
    bool erase(const T& value);
    void clear();

    // 查询
    bool contains(const T& value) const;
    int rank(const T& value) const;
    T& kth(int k);
    const T& kth(int k) const;

    // 基础增强操作
    iterator predecessor(const T& value);
    const_iterator predecessor(const T& value) const;
    iterator successor(const T& value);
    const_iterator successor(const T& value) const;
    std::vector<T> range_query(const T& low, const T& high) const;
    void range_erase(const T& low, const T& high);

    // 迭代器接口
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;
    reverse_iterator rbegin();
    reverse_iterator rend();
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;
    const_reverse_iterator crbegin() const;
    const_reverse_iterator crend() const;

    // 调试
    void inorder() const;

private:
    // 节点结构声明（定义在 .inl 中）
    struct Node;
    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAllocator>;

    Node* root;
    Compare comp;
    typename std::allocator_traits<Allocator>::template rebind_alloc<Node> node_alloc;

    // 私有辅助函数声明
    int getHeight(Node* node) const;
    int getSize(Node* node) const;
    void updateHeight(Node* node);
    void updateSize(Node* node);
    int getBalance(Node* node) const;

    Node* rotateRight(Node* y);
    Node* rotateLeft(Node* x);
    Node* balance(Node* node);

    Node* createNode(const T& value);
    void destroyNode(Node* node);

    Node* insertRec(Node* node, const T& value);
    Node* eraseRec(Node* node, const T& value);
    Node* findNode(const T& value) const;
    Node* findMin(Node* node) const;
    void clearRec(Node* node);

    int rankRec(Node* node, const T& value) const;
    Node* kthRec(Node* node, int k) const;

    void range_query_rec(Node* node, const T& low, const T& high, std::vector<T>& out) const;
    Node* range_erase_rec(Node* node, const T& low, const T& high);

    void inorderRec(Node* node) const;

    // 迭代器类声明为友元，以便访问私有节点
    friend class AVLTreeIterator<T, Compare, Allocator, false>;
    friend class AVLTreeIterator<T, Compare, Allocator, true>;
};

// 迭代器模板声明
template <typename T, typename Compare, typename Allocator, bool IsConst>
class AVLTreeIterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::conditional<IsConst, const T*, T*>::type;
    using reference = typename std::conditional<IsConst, const T&, T&>::type;

    AVLTreeIterator(typename AVLTree<T, Compare, Allocator>::Node* node = nullptr);

    // 允许从 iterator 到 const_iterator 的转换
    template <bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
    AVLTreeIterator(const AVLTreeIterator<T, Compare, Allocator, OtherConst>& other);

    reference operator*() const;
    pointer operator->() const;

    AVLTreeIterator& operator++();
    AVLTreeIterator operator++(int);
    AVLTreeIterator& operator--();
    AVLTreeIterator operator--(int);

    bool operator==(const AVLTreeIterator& other) const;
    bool operator!=(const AVLTreeIterator& other) const;

    // 供反向迭代器访问底层指针
    typename AVLTree<T, Compare, Allocator>::Node* base() const;

private:
    typename AVLTree<T, Compare, Allocator>::Node* ptr;
    void increment();
    void decrement();
};

#include "AVLTree.inl"   // 包含模板定义

#endif // AVLTREE_H