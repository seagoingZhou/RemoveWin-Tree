# 无冲突复制Set数据类型

无冲突复制Set数据类型各新增了5条命令，其格式和含义为：
* **[type]SADD key member [member …]** : 将一个或多个 *member*元素加入到集合*key*当中，已经存在于集合的*member*元素将被忽略。
* **[type]SREM key member [member …]** : 移除集合*key*中的一个或多个*member*元素，不存在的*member*元素会被忽略。
* **[type]SINTERSTORE destination key [key …]** : 计算给定集合的交集，将结果保存到*destination*集合。
* **[type]SUNIONSTORE destination key [key …]** : 计算给定集合的并集，将结果保存到*destination*集合。
* **[type]SDIFFSTORE destination key [key …]** : 计算给定集合的差集，将结果保存到*destination*集合。

##RWF-Set
根据算法设计，实现了RWF-Set

- 结构设计：一个 RWF-Set 在实际存储为两部分
    - 存储存在的元素集合，使用 redis 原有的 set 底层实现。对外表现的集合。
    - 存储元素信息的哈希表使用 redis 原有的 hash set 底层实现。哈希表在有序集合的名字基础上加一个后缀存储，对外不可见。由于哈希表的设计中值只能是字符串，因此将自定数据结构指针值当作字符串存入其中。
- 服务于该 RPQ 的新结构体及其操作
    * *rwse* : RWF-Set的元数据，存储有remove-history
- 实用函数/宏：
    * *sremGenericCommand*，元素删除操作的**effct**阶段的通用操作，解析CRDT广播命令的参数，根据算法的逻辑执行元素删除操作。
    * *saddGenericCommand*，元素添加操作的**effct**阶段的通用操作，解析CRDT广播命令的参数，根据算法的逻辑执行元素添加操作。
    * *rwseHTGet*，获得元素存储数据结构，不存在则依据参数是否创建。
    * *getSetOrCreate*，查找集合，没有则创建。

##OR-Set
根据算法设计，实现了OR-Set

- 结构设计：一个 RWF-Set 在实际存储为两部分
    - 存储存在的元素集合，使用 redis 原有的 set 底层实现。对外表现的集合。
    - 存储元素信息的哈希表使用 redis 原有的 hash set 底层实现。哈希表在有序集合的名字基础上加一个后缀存储，对外不可见。由于哈希表的设计中值只能是字符串，因此将自定数据结构指针值当作字符串存入其中。
- 服务于该 RPQ 的新结构体及其操作
    * *ore* : OR-Set的元数据，存储有添加操作的tag集合、删除操作的tag集合和当前添加操作计数器
- 实用函数/宏：
    * *tagGenerate*，tag生成函数
    * *lookup*，判断元素是否存在。
    * *orsremGenericCommand*，元素删除操作的**effct**阶段的通用操作，解析CRDT广播命令的参数，根据算法的逻辑执行元素删除操作。
    * *orsaddGenericCommand*，元素添加操作的**effct**阶段的通用操作，解析CRDT广播命令的参数，根据算法的逻辑执行元素添加操作。
    * *oreHTGet*，获得元素存储数据结构，不存在则依据参数是否创建。

##PN-Set
根据算法设计，实现了PN-Set

- 结构设计：一个 PN-Set 在实际存储为两部分
    - 存储存在的元素集合，使用 redis 原有的 set 底层实现。对外表现的集合。
    - 存储元素信息的哈希表使用 redis 原有的 hash set 底层实现。哈希表在有序集合的名字基础上加一个后缀存储，对外不可见。由于哈希表的设计中值只能是字符串，因此将自定数据结构指针值当作字符串存入其中。
- 服务于该 RPQ 的新结构体及其操作
    * *pne* : PN-Set的元数据，存储有计数器
- 实用函数/宏：
    * *pnsremGenericCommand*，元素删除操作的**effct**阶段的通用操作，解析CRDT广播命令的参数，根据算法的逻辑执行元素删除操作。
    * *pnsaddGenericCommand*，元素添加操作的**effct**阶段的通用操作，解析CRDT广播命令的参数，根据算法的逻辑执行元素添加操作。
    * *pneHTGet*，获得元素存储数据结构，不存在则依据参数是否创建。