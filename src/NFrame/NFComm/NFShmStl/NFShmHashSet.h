// -------------------------------------------------------------------------
//    @FileName         :    NFShmHashSet.h
//    @Author           :    gaoyi
//    @Date             :    23-2-11
//    @Email			:    445267987@qq.com
//    @Module           :    NFShmHashSet
//
// -------------------------------------------------------------------------

#pragma once

#include "NFShmHashTable.h"
#include <set>
#include <unordered_set>

/**
 * @file NFShmHashSet.h
 * @brief 基于共享内存的无序集合容器实现，与STL std::unordered_set高度兼容
 * 
 * @section overview 概述
 * 
 * NFShmHashSet 是一个专为共享内存环境设计的无序集合容器，保证元素的唯一性。
 * 在API设计上与STL std::unordered_set高度兼容，但在内存管理、容量控制和性能特征方面
 * 针对共享内存场景进行了深度优化。它提供O(1)平均时间复杂度的操作，同时支持进程间数据共享。
 * 
 * @section features 核心特性
 * 
 * 1. **集合语义**：
 *    - 保证元素唯一性，不允许重复元素
 *    - 插入重复元素时返回现有元素的迭代器
 *    - 支持高效的成员检查（contains语义）
 *    - 基于哈希的快速查找和插入
 * 
 * 2. **STL高度兼容**：
 *    - 完整的std::unordered_set API支持
 *    - 标准的迭代器接口和类型定义
 *    - find()、count()、insert()等常用操作
 *    - 支持范围for循环和STL算法
 * 
 * 3. **共享内存优化**：
 *    - 固定大小内存布局，避免动态分配
 *    - 元素直接存储，无额外包装
 *    - 支持CREATE/RESUME两阶段初始化
 *    - 无内存碎片，高效的内存使用
 * 
 * 4. **性能特征**：
 *    - O(1)平均时间复杂度的哈希操作
 *    - 无rehash开销，稳定的性能表现
 *    - 链地址法解决冲突，固定桶数量
 *    - 预分配节点池，快速内存分配
 * 
 * @section stl_comparison 与STL std::unordered_set对比
 * 
 * | 特性 | std::unordered_set | NFShmHashSet |
 * |------|-------------------|--------------|
 * | **内存管理** | 动态堆内存分配 | 固定共享内存预分配 |
 * | **容量限制** | 无限制，动态扩容 | 固定容量MAX_SIZE |
 * | **扩容机制** | 自动rehash扩容 | **不支持扩容** |
 * | **元素存储** | 直接存储元素值 | 直接存储元素值 |
 * | **唯一性保证** | 完全保证 | 完全保证 |
 * | **插入语义** | 重复元素插入失败 | 重复元素插入失败 |
 * | **查找性能** | O(1)平均，O(n)最坏 | O(1)平均，O(n)最坏 |
 * | **成员检查** | find() != end() | find() != end() |
 * | **count操作** | 返回0或1 | 返回0或1 |
 * | **进程共享** | 不支持 | **原生支持** |
 * | **初始化方式** | 构造函数 | CreateInit/ResumeInit |
 * | **负载因子** | 动态调整 | 固定结构 |
 * | **异常安全** | 强异常安全保证 | 无异常，错误码返回 |
 * | **内存碎片** | 可能产生 | **无碎片** |
 * | **性能稳定性** | rehash时性能抖动 | **性能稳定** |
 * | **迭代器稳定性** | rehash时失效 | **始终稳定** |
 * 
 * @section api_compatibility API兼容性
 * 
 * **完全兼容的接口**：
 * - **容器属性**：size(), empty(), max_size()
 * - **迭代器**：begin(), end(), cbegin(), cend()
 * - **查找访问**：find(), count()
 * - **修改操作**：insert(), emplace(), erase(), clear()
 * - **容器操作**：swap()
 * - **桶接口**：bucket_count(), max_bucket_count(), elems_in_bucket()
 * 
 * **扩展的接口（新增）**：
 * - **容量检查**：full(), left_size()
 * - **共享内存**：CreateInit(), ResumeInit()
 * - **STL转换**：从std::unordered_set和std::set构造
 * 
 * **不支持的接口**：
 * - **动态管理**：rehash(), reserve(), max_load_factor()
 * - **哈希策略**：load_factor(), bucket_size()
 * - **自定义分配器**：get_allocator()
 * 
 * @section usage_examples 使用示例
 * 
 * @subsection basic_usage 基础用法（类似std::unordered_set）
 * 
 * ```cpp
 * // 定义容量为1000的整数集合
 * NFShmHashSet<int, 1000> numberSet;
 * numberSet.CreateInit();  // 创建模式初始化
 * 
 * // 插入元素（保证唯一性）
 * auto result1 = numberSet.insert(42);
 * std::cout << "Insert 42: " << (result1.second ? "success" : "already exists") << std::endl;
 * 
 * auto result2 = numberSet.insert(42);  // 重复插入
 * std::cout << "Insert 42 again: " << (result2.second ? "success" : "already exists") << std::endl;
 * 
 * // 批量插入
 * std::vector<int> numbers = {1, 2, 3, 2, 4, 3, 5};  // 包含重复元素
 * numberSet.insert(numbers.begin(), numbers.end());
 * 
 * std::cout << "Set size: " << numberSet.size() << std::endl;  // 输出：6 (去重后)
 * 
 * // 成员检查
 * if (numberSet.find(42) != numberSet.end()) {
 *     std::cout << "42 is in the set" << std::endl;
 * }
 * 
 * // count操作（对于集合，只返回0或1）
 * std::cout << "Count of 42: " << numberSet.count(42) << std::endl;  // 输出：1
 * std::cout << "Count of 99: " << numberSet.count(99) << std::endl;  // 输出：0
 * 
 * // 遍历集合
 * std::cout << "Elements: ";
 * for (const auto& num : numberSet) {
 *     std::cout << num << " ";
 * }
 * std::cout << std::endl;
 * ```
 * 
 * @subsection set_operations 集合操作
 * 
 * ```cpp
 * NFShmHashSet<std::string, 500> wordsSet;
 * 
 * // 构建单词集合
 * std::vector<std::string> words = {
 *     "apple", "banana", "apple", "cherry", "banana", "date"
 * };
 * 
 * for (const auto& word : words) {
 *     auto result = wordsSet.insert(word);
 *     if (!result.second) {
 *         std::cout << "Duplicate word ignored: " << word << std::endl;
 *     }
 * }
 * 
 * std::cout << "Unique words: " << wordsSet.size() << std::endl;  // 输出：4
 * 
 * // 集合成员检查函数
 * auto contains = [&](const std::string& word) -> bool {
 *     return wordsSet.find(word) != wordsSet.end();
 * };
 * 
 * // 检查单词是否存在
 * std::vector<std::string> checkWords = {"apple", "grape", "cherry", "orange"};
 * for (const auto& word : checkWords) {
 *     std::cout << word << ": " << (contains(word) ? "exists" : "not found") << std::endl;
 * }
 * 
 * // 删除元素
 * size_t removed = wordsSet.erase("apple");
 * std::cout << "Removed " << removed << " element(s)" << std::endl;  // 输出：1
 * 
 * // 条件删除（使用迭代器）
 * for (auto it = wordsSet.begin(); it != wordsSet.end();) {
 *     if (it->length() > 5) {  // 删除长度超过5的单词
 *         it = wordsSet.erase(it);
 *     } else {
 *         ++it;
 *     }
 * }
 * ```
 * 
 * @subsection capacity_management 容量管理
 * 
 * ```cpp
 * NFShmHashSet<int, 100> limitedSet;
 * 
 * // 容量检查（STL没有的功能）
 * std::cout << "Max size: " << limitedSet.max_size() << std::endl;      // 100
 * std::cout << "Current size: " << limitedSet.size() << std::endl;       // 0
 * std::cout << "Is full: " << limitedSet.full() << std::endl;           // false
 * std::cout << "Left space: " << limitedSet.left_size() << std::endl;   // 100
 * 
 * // 批量插入直到容量满
 * for (int i = 0; i < 150; ++i) {
 *     auto result = limitedSet.insert(i);
 *     if (!result.second) {
 *         std::cout << "Insert failed at " << i << ", set is full" << std::endl;
 *         break;  // 容量达到100时插入失败
 *     }
 * }
 * 
 * // 检查最终状态
 * std::cout << "Final size: " << limitedSet.size() << std::endl;         // 100
 * std::cout << "Is full: " << limitedSet.full() << std::endl;           // true
 * std::cout << "Left space: " << limitedSet.left_size() << std::endl;   // 0
 * 
 * // 验证唯一性
 * std::cout << "All elements are unique: " << std::boolalpha 
 *           << (limitedSet.size() == 100) << std::endl;  // true
 * ```
 * 
 * @subsection shared_memory_usage 共享内存使用
 * 
 * ```cpp
 * // 进程A：创建共享的集合
 * NFShmHashSet<int, 1000> sharedSet;
 * if (sharedSet.CreateInit() == 0) {  // 创建成功
 *     // 添加一些数据
 *     std::vector<int> data = {1, 2, 3, 4, 5};
 *     sharedSet.insert(data.begin(), data.end());
 *     std::cout << "Created shared set with " << sharedSet.size() << " elements" << std::endl;
 * }
 * 
 * // 进程B：恢复已存在的共享内存集合
 * NFShmHashSet<int, 1000> restoredSet;
 * if (restoredSet.ResumeInit() == 0) {  // 恢复成功
 *     std::cout << "Restored set with " << restoredSet.size() << " elements" << std::endl;
 *     
 *     // 检查进程A创建的数据
 *     for (int i = 1; i <= 5; ++i) {
 *         if (restoredSet.count(i) > 0) {
 *             std::cout << "Element " << i << " exists from Process A" << std::endl;
 *         }
 *     }
 *     
 *     // 添加进程B的数据
 *     std::vector<int> newData = {6, 7, 8, 9, 10};
 *     restoredSet.insert(newData.begin(), newData.end());
 *     
 *     std::cout << "After Process B additions: " << restoredSet.size() << " elements" << std::endl;
 * }
 * ```
 * 
 * @subsection stl_interop STL容器互操作
 * 
 * ```cpp
 * // 从STL容器构造
 * std::unordered_set<std::string> stdSet = {
 *     "apple", "banana", "cherry", "apple"  // 注意：apple重复，STL会自动去重
 * };
 * 
 * NFShmHashSet<std::string, 1000> shmSet(stdSet);  // 从STL构造
 * std::cout << "Converted from STL: " << shmSet.size() << " elements" << std::endl;
 * 
 * // 转换回STL容器
 * std::unordered_set<std::string> convertedSet;
 * for (const auto& element : shmSet) {
 *     convertedSet.insert(element);
 * }
 * 
 * // 使用STL算法
 * auto count = std::count_if(shmSet.begin(), shmSet.end(),
 *     [](const std::string& word) { return word.length() > 5; });
 * std::cout << "Words longer than 5 chars: " << count << std::endl;
 * 
 * // 集合运算（模拟）
 * NFShmHashSet<int, 100> setA, setB;
 * setA.insert({1, 2, 3, 4, 5});
 * setB.insert({4, 5, 6, 7, 8});
 * 
 * // 交集（手动实现）
 * std::vector<int> intersection;
 * for (const auto& element : setA) {
 *     if (setB.count(element) > 0) {
 *         intersection.push_back(element);
 *     }
 * }
 * 
 * std::cout << "Intersection size: " << intersection.size() << std::endl;  // 输出：2 (4,5)
 * ```
 * 
 * @section performance_notes 性能说明
 * 
 * - **查找性能**：O(1)平均，O(n)最坏（链表长度）
 * - **插入性能**：O(1)平均，重复元素快速拒绝
 * - **删除性能**：O(1)平均，节点回收到内存池
 * - **唯一性检查**：插入时自动进行，无额外开销
 * - **内存性能**：零碎片，预分配，缓存友好
 * - **并发性能**：需要外部同步，但支持多进程读写
 * 
 * @section migration_guide 迁移指南
 * 
 * 从std::unordered_set迁移到NFShmHashSet：
 * 
 * 1. **替换类型声明**：
 *    ```cpp
 *    // 原代码
 *    std::unordered_set<int> set;
 *    
 *    // 新代码（添加容量模板参数）
 *    NFShmHashSet<int, 10000> set;
 *    ```
 * 
 * 2. **添加初始化**：
 *    ```cpp
 *    // 添加共享内存初始化
 *    set.CreateInit();  // 或 ResumeInit()
 *    ```
 * 
 * 3. **保持集合语义**：
 *    ```cpp
 *    // 集合的核心操作保持不变
 *    auto result = set.insert(element);
 *    bool inserted = result.second;  // 是否成功插入
 *    
 *    bool exists = set.count(element) > 0;  // 成员检查
 *    auto it = set.find(element);           // 查找元素
 *    ```
 * 
 * 4. **处理容量限制**：
 *    ```cpp
 *    // 检查容量状态
 *    if (!set.full()) {
 *        set.insert(element);
 *    } else {
 *        // 处理容量满的情况
 *    }
 *    ```
 * 
 * 5. **移除不支持的操作**：
 *    ```cpp
 *    // 移除这些调用（NFShmHashSet不支持）
 *    // set.rehash(1000);     // 动态管理不支持
 *    // set.reserve(500);     // 动态管理不支持
 *    // set.max_load_factor(0.8);  // 负载因子管理不支持
 *    ```
 */

/****************************************************************************
 * STL std::unordered_set 对比分析
 ****************************************************************************
 * 
 * 1. 内存管理策略对比：
 *    - std::unordered_set: 动态内存分配，使用allocator管理堆内存
 *    - NFShmHashSet: 固定大小共享内存，预分配所有节点，支持进程间共享
 * 
 * 2. 容量管理对比：
 *    - std::unordered_set: 动态扩容，load_factor超过阈值时自动rehash
 *    - NFShmHashSet: 固定容量MAX_SIZE，不支持动态扩容
 * 
 * 3. 元素存储对比：
 *    - std::unordered_set: 直接存储元素值
 *    - NFShmHashSet: 同样直接存储元素值，使用std::_Identity函数提取键
 * 
 * 4. 唯一性保证对比：
 *    - 都保证元素唯一性，不允许重复元素
 *    - 插入重复元素时都返回现有元素的迭代器
 * 
 * 5. 性能特征对比：
 *    - std::unordered_set: O(1)平均时间复杂度，但可能触发rehash开销
 *    - NFShmHashSet: O(1)平均时间复杂度，无rehash开销，但可能因固定容量产生更多冲突
 * 
 * 6. API兼容性：
 *    - 兼容接口：insert, find, erase, begin, end, size, empty, count等
 *    - 特有接口：full(), left_size(), CreateInit(), ResumeInit()等共享内存特有功能
 *    - 缺少接口：rehash, reserve, bucket, load_factor等动态管理接口
 * 
 * 7. 构造和初始化对比：
 *    - std::unordered_set: 标准构造函数，支持多种初始化方式
 *    - NFShmHashSet: 支持从STL容器构造，增加共享内存特有的CreateInit/ResumeInit
 *****************************************************************************/

// ==================== 前向声明 ====================

template <class Value, int MAX_SIZE,
          class HashFcn = std::hash<Value>,
          class EqualKey = std::equal_to<Value>>
class NFShmHashSet;

template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
bool operator==(const NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>& hs2);

// ==================== 主要容器类 ====================

/**
 * @brief 基于共享内存的无序集合容器
 * @tparam Value 元素类型
 * @tparam MAX_SIZE 最大容量（固定）
 * @tparam HashFcn 哈希函数类型，默认std::hash<Value>
 * @tparam EqualKey 元素比较函数类型，默认std::equal_to<Value>
 * 
 * 设计特点：
 * 1. 固定容量，不支持动态扩容（与STL主要区别）
 * 2. 基于共享内存，支持进程间共享
 * 3. API设计尽量兼容STL std::unordered_set
 * 4. 保证元素唯一性，不允许重复元素
 * 
 * 与std::unordered_set的主要差异：
 * - 容量限制：固定大小 vs 动态扩容
 * - 内存管理：共享内存 vs 堆内存
 * - 性能特征：无rehash开销 vs 动态性能优化
 * - 进程支持：进程间共享 vs 单进程内使用
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
class NFShmHashSet
{
private:
    /// @brief 底层哈希表类型，使用std::_Identity函数提取键（键即值本身）
    typedef NFShmHashTable<Value, Value, MAX_SIZE, HashFcn, std::stl__Identity<Value>, EqualKey> HashTable;
    HashTable m_hashTable; ///< 底层哈希表实例

public:
    // ==================== STL兼容类型定义 ====================

    typedef typename HashTable::key_type key_type; ///< 键类型（与值类型相同）
    typedef typename HashTable::value_type value_type; ///< 值类型（即元素类型）
    typedef typename HashTable::hasher hasher; ///< 哈希函数类型
    typedef typename HashTable::key_equal key_equal; ///< 键相等比较函数类型

    typedef typename HashTable::size_type size_type; ///< 大小类型
    typedef typename HashTable::difference_type difference_type; ///< 差值类型
    typedef typename HashTable::pointer pointer; ///< 指针类型
    typedef typename HashTable::const_pointer const_pointer; ///< 常量指针类型
    typedef typename HashTable::reference reference; ///< 引用类型
    typedef typename HashTable::const_reference const_reference; ///< 常量引用类型

    typedef typename HashTable::iterator iterator; ///< 迭代器类型
    typedef typename HashTable::const_iterator const_iterator; ///< 常量迭代器类型

public:
    // ==================== 构造函数和析构函数 ====================

    /**
     * @brief 默认构造函数
     * @note 根据SHM_CREATE_MODE决定创建或恢复模式
     * @note 与std::unordered_set()行为类似，但增加共享内存初始化
     */
    NFShmHashSet()
    {
        if (SHM_CREATE_MODE)
        {
            CreateInit();
        }
        else
        {
            ResumeInit();
        }
    }

    /**
     * @brief 范围构造函数
     * @tparam InputIterator 输入迭代器类型
     * @param f 起始迭代器
     * @param l 结束迭代器
     * @note 与std::unordered_set(first, last)兼容
     */
    template <class InputIterator>
    NFShmHashSet(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 数组构造函数
     * @param f 起始指针
     * @param l 结束指针
     * @note 与std::unordered_set兼容
     */
    NFShmHashSet(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 迭代器构造函数
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    NFShmHashSet(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 拷贝构造函数
     * @param x 源NFShmHashSet对象
     * @note 与std::unordered_set拷贝构造函数兼容
     */
    NFShmHashSet(const NFShmHashSet& x)
    {
        if (this != &x) m_hashTable = x.m_hashTable;
    }

    /**
     * @brief 从std::unordered_set构造
     * @param set 源std::unordered_set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashSet(const std::unordered_set<Value>& set)
    {
        m_hashTable.insert_unique(set.begin(), set.end());
    }

    /**
     * @brief 从std::set构造
     * @param set 源std::set对象
     * @note STL容器没有此构造函数，为方便互操作而提供
     */
    explicit NFShmHashSet(const std::set<Value>& set)
    {
        m_hashTable.insert_unique(set.begin(), set.end());
    }

    /**
     * @brief 初始化列表构造函数
     * @param list 初始化列表
     * @note 与std::unordered_set(std::initializer_list)兼容
     */
    NFShmHashSet(const std::initializer_list<Value>& list)
    {
        insert(list.begin(), list.end());
    }

    // ==================== 共享内存特有接口 ====================

    /**
     * @brief 创建模式初始化
     * @return 0表示成功
     * @note STL容器没有此接口，共享内存特有
     */
    int CreateInit()
    {
        return 0;
    }

    /**
     * @brief 恢复模式初始化
     * @return 0表示成功
     * @note 从共享内存恢复时调用，STL容器没有此接口
     */
    int ResumeInit()
    {
        return 0;
    }

    /**
     * @brief 就地初始化
     * @note 使用placement new重新构造对象
     * @note STL容器没有此接口
     */
    void Init()
    {
        new(this) NFShmHashSet();
    }

    // ==================== 赋值操作符 ====================

    /**
     * @brief 拷贝赋值操作符
     * @param x 源NFShmHashSet对象
     * @return 自身引用
     * @note 与std::unordered_set::operator=兼容
     */
    NFShmHashSet<Value, MAX_SIZE>& operator=(const NFShmHashSet<Value, MAX_SIZE>& x);

    /**
     * @brief 从std::unordered_set赋值
     * @param x 源std::unordered_set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashSet<Value, MAX_SIZE>& operator=(const std::unordered_set<Value>& x);

    /**
     * @brief 从std::set赋值
     * @param x 源std::set对象
     * @return 自身引用
     * @note STL容器没有此接口，为方便互操作而提供
     */
    NFShmHashSet<Value, MAX_SIZE>& operator=(const std::set<Value>& x);

    /**
     * @brief 从初始化列表赋值
     * @param x 初始化列表
     * @return 自身引用
     * @note 与std::unordered_set::operator=(std::initializer_list)兼容
     */
    NFShmHashSet<Value, MAX_SIZE>& operator=(const std::initializer_list<Value>& x);

public:
    // ==================== 容量相关接口（STL兼容） ====================

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     * @note 与std::unordered_set::size()兼容
     */
    size_type size() const { return m_hashTable.size(); }

    /**
     * @brief 获取最大容量
     * @return 最大容量MAX_SIZE
     * @note 与std::unordered_set::max_size()不同，返回固定值而非理论最大值
     */
    size_type max_size() const { return m_hashTable.max_size(); }

    /**
     * @brief 判断是否为空
     * @return true表示空
     * @note 与std::unordered_set::empty()兼容
     */
    bool empty() const { return m_hashTable.empty(); }

    /**
     * @brief 交换两个容器的内容
     * @param hs 另一个NFShmHashSet对象
     * @note 与std::unordered_set::swap()兼容
     */
    void swap(NFShmHashSet& hs) noexcept { m_hashTable.swap(hs.m_hashTable); }

    /**
     * @brief 判断是否已满
     * @return true表示已满
     * @note STL容器没有此接口，固定容量特有
     */
    bool full() const { return m_hashTable.full(); }

    /**
     * @brief 获取剩余容量
     * @return 剩余可用空间
     * @note STL容器没有此接口，固定容量特有
     */
    size_t left_size() const { return m_hashTable.left_size(); }

    // ==================== 友元比较操作符 ====================

    template <class Val, int X_MAX_SIZE, class Hf, class EqK>
    friend bool operator==(const NFShmHashSet<Val, X_MAX_SIZE, Hf, EqK>&,
                           const NFShmHashSet<Val, X_MAX_SIZE, Hf, EqK>&);

    // ==================== STL兼容迭代器接口 ====================

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     * @note 与std::unordered_set::begin()兼容
     */
    iterator begin() { return m_hashTable.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     * @note 与std::unordered_set::end()兼容
     */
    iterator end() { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_set::begin() const兼容
     */
    const_iterator begin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_set::end() const兼容
     */
    const_iterator end() const { return m_hashTable.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     * @note 与std::unordered_set::cbegin()兼容
     */
    const_iterator cbegin() const { return m_hashTable.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     * @note 与std::unordered_set::cend()兼容
     */
    const_iterator cend() const { return m_hashTable.end(); }

public:
    // ==================== 插入接口（STL兼容） ====================

    /**
     * @brief 插入元素
     * @param obj 要插入的元素
     * @return pair<iterator, bool>，迭代器指向元素，bool表示是否插入成功
     * @note 与std::unordered_set::insert()兼容
     */
    std::pair<iterator, bool> insert(const value_type& obj)
    {
        return m_hashTable.insert_unique(obj);
    }

    /**
     * @brief 带提示插入元素
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param obj 要插入的元素
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_set::insert(const_iterator, const value_type&)兼容
     */
    iterator insert(const_iterator hint, const value_type& obj)
    {
        (void)hint; // 忽略提示参数
        return m_hashTable.insert_unique(obj).first;
    }

    /**
     * @brief 就地构造元素
     * @tparam Args 构造参数类型包
     * @param args 构造参数
     * @return pair<iterator, bool>
     * @note 与std::unordered_set::emplace()兼容
     */
    template <typename... Args>
    std::pair<iterator, bool> emplace(const Args&... args);

    /**
     * @brief 带提示就地构造元素
     * @tparam Args 构造参数类型包
     * @param hint 位置提示迭代器（本实现中忽略）
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     * @note 与std::unordered_set::emplace_hint()兼容
     */
    template <typename... Args>
    iterator emplace_hint(const_iterator hint, const Args&... args);

    /**
     * @brief 范围插入
     * @tparam InputIterator 输入迭代器类型
     * @param f 起始迭代器
     * @param l 结束迭代器
     * @note 与std::unordered_set::insert(first, last)兼容
     */
    template <class InputIterator>
    void insert(InputIterator f, InputIterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 数组范围插入
     * @param f 起始指针
     * @param l 结束指针
     */
    void insert(const value_type* f, const value_type* l)
    {
        m_hashTable.insert_unique(f, l);
    }

    /**
     * @brief 迭代器范围插入
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     */
    void insert(const_iterator f, const_iterator l)
    {
        m_hashTable.insert_unique(f, l);
    }

    // ==================== 查找接口（STL兼容） ====================

    /**
     * @brief 查找元素
     * @param key 要查找的键（元素值）
     * @return 指向元素的迭代器，未找到返回end()
     * @note 与std::unordered_set::find()兼容
     */
    iterator find(const key_type& key)
    {
        return m_hashTable.find(key);
    }

    const_iterator find(const key_type& key) const
    {
        return m_hashTable.find(key);
    }

    /**
     * @brief 统计指定键的元素数量
     * @param key 要统计的键
     * @return 元素数量（0或1，因为是unique容器）
     * @note 与std::unordered_set::count()兼容
     */
    size_type count(const key_type& key) const
    {
        return m_hashTable.count(key);
    }

    /**
     * @brief 获取指定键的元素范围
     * @param key 要查找的键
     * @return pair<iterator, iterator>表示范围
     * @note 与std::unordered_set::equal_range()兼容
     */
    std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const
    {
        return m_hashTable.equal_range(key);
    }

    std::pair<iterator, iterator> equal_range(const key_type& key)
    {
        return m_hashTable.equal_range(key);
    }

    // ==================== 删除接口（STL兼容） ====================

    /**
     * @brief 根据键删除元素
     * @param key 要删除的键
     * @return 删除的元素数量
     * @note 与std::unordered_set::erase()兼容
     */
    size_type erase(const key_type& key)
    {
        return m_hashTable.erase(key);
    }

    /**
     * @brief 根据迭代器删除元素
     * @param it 指向要删除元素的迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_set::erase()兼容
     */
    iterator erase(iterator it)
    {
        return m_hashTable.erase(it);
    }

    /**
     * @brief 根据常量迭代器删除元素
     * @param it 指向要删除元素的常量迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_set::erase()兼容
     */
    iterator erase(const_iterator it)
    {
        return m_hashTable.erase(it);
    }

    /**
     * @brief 删除指定范围的元素
     * @param f 起始常量迭代器
     * @param l 结束常量迭代器
     * @return 指向下一个元素的迭代器
     * @note 与std::unordered_set::erase(first, last)兼容
     */
    iterator erase(const_iterator f, const_iterator l)
    {
        return m_hashTable.erase(f, l);
    }

    /**
     * @brief 清空所有元素
     * @note 与std::unordered_set::clear()兼容
     */
    void clear()
    {
        m_hashTable.clear();
    }

public:
    // ==================== 桶接口（STL兼容） ====================

    /**
     * @brief 调整大小提示
     * @param hint 元素数量提示
     * @note 与std::unordered_set::rehash()类似但实际不执行操作（固定容量）
     */
    void resize(size_type hint)
    {
        m_hashTable.resize(hint);
    }

    /**
     * @brief 获取桶数量
     * @return 桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_set::bucket_count()兼容，但返回固定值
     */
    size_type bucket_count() const
    {
        return m_hashTable.bucket_count();
    }

    /**
     * @brief 获取最大桶数量
     * @return 最大桶数量（固定为MAX_SIZE）
     * @note 与std::unordered_set::max_bucket_count()兼容
     */
    size_type max_bucket_count() const
    {
        return m_hashTable.max_bucket_count();
    }

    /**
     * @brief 获取指定桶中的元素数量
     * @param n 桶索引
     * @return 该桶中的元素数量
     * @note 与std::unordered_set::bucket_size()类似
     */
    size_type elems_in_bucket(size_type n) const
    {
        return m_hashTable.elems_in_bucket(n);
    }
};

// ==================== 实现部分 ====================

/**
 * @brief 从std::set赋值实现
 * @param x 源std::set对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从有序容器转换而提供
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashSet<Value, MAX_SIZE>& NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::set<Value>& x)
{
    clear();
    m_hashTable.insert_unique(x.begin(), x.end());
    return *this;
}

/**
 * @brief 从std::unordered_set赋值实现
 * @param x 源std::unordered_set对象
 * @return 自身引用
 * @note STL容器没有此接口，为方便从STL无序容器转换而提供
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashSet<Value, MAX_SIZE>& NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::unordered_set<Value>& x)
{
    clear();
    m_hashTable.insert_unique(x.begin(), x.end());
    return *this;
}

/**
 * @brief 拷贝赋值操作符实现
 * @param x 源NFShmHashSet对象
 * @return 自身引用
 * @note 与std::unordered_set::operator=兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashSet<Value, MAX_SIZE>& NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const NFShmHashSet<Value, MAX_SIZE>& x)
{
    if (this != &x)
    {
        clear();
        m_hashTable = x.m_hashTable;
    }
    return *this;
}

/**
 * @brief 从初始化列表赋值实现
 * @param x 初始化列表
 * @return 自身引用
 * @note 与std::unordered_set::operator=(std::initializer_list)兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
NFShmHashSet<Value, MAX_SIZE>& NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>::operator=(const std::initializer_list<Value>& x)
{
    clear();
    m_hashTable.insert_unique(x.begin(), x.end());
    return *this;
}

/**
 * @brief 就地构造元素实现
 * @tparam Args 构造参数类型包
 * @param args 构造参数
 * @return pair<iterator, bool>，迭代器指向元素，bool表示是否插入成功
 * @note 与std::unordered_set::emplace()兼容
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename... Args>
std::pair<typename NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>::iterator, bool>
NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>::emplace(const Args&... args)
{
    value_type obj(args...);
    return m_hashTable.insert_unique(obj);
}

/**
 * @brief 带提示就地构造元素实现
 * @tparam Args 构造参数类型包
 * @param hint 位置提示迭代器（忽略）
 * @param args 构造参数
 * @return 指向插入元素的迭代器
 * @note 与std::unordered_set::emplace_hint()兼容，但忽略提示参数
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
template <typename... Args>
typename NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>::iterator
NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>::emplace_hint(const_iterator hint, const Args&... args)
{
    (void)hint; // 忽略提示参数
    value_type obj(args...);
    return m_hashTable.insert_unique(obj).first;
}

// ==================== 全局操作符 ====================

/**
 * @brief 相等比较操作符
 * @param hs1 第一个NFShmHashSet对象
 * @param hs2 第二个NFShmHashSet对象
 * @return true表示相等
 * @note 与std::unordered_set的operator==兼容
 * @note 比较所有元素是否相等，顺序无关
 */
template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
bool operator==(const NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>& hs2)
{
    return hs1.m_hashTable == hs2.m_hashTable;
}

template <class Value, int MAX_SIZE, class HashFcn, class EqualKey>
void swap(const NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>& hs1,
                const NFShmHashSet<Value, MAX_SIZE, HashFcn, EqualKey>& hs2)
{
    hs1.m_hashTable.swap(hs2.m_hashTable);
}
