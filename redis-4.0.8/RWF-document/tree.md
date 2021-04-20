# 无冲突复制Tree数据类型

## 新增命令
RWF-Tree新增了6条命令，其格式和含义为：
* **RWFTREECREATE tree\_name** : 创建一个名为*tree\_name*的RWF-Tree。
* **RWFTREEINSERT  tree\_name value parent\_id id** : 向RWF-Tree *tree\_name*中id为*parent\_id*的节点插入一个子节点，该子节点的id为*id*，value为*value*
* **RWFTREEDELETE tree\_name id** 删除RWF-Tree *tree\_name*中id为*id*的节点及其所有子节点
* **RWFTREECHANGEVALUE tree\_name id value** 将RWF-Tree *tree\_name*中id为*id*的节点的value值修改为*value*
* **RWFTREEMOVE tree\_name destination\_id  source\_id** 移动RWF-Tree *tree\_name*中id为*source\_id*的节点及其所有子节点到id为*destination\_id*的节点下，使得节点*destination\_id*为节点*source\_id*的父节点
* **RWFTREEMEMBERS tree\_name** 返回RWF-Tree *tree\_name*中所有成员及其属性信息

## 设计细节

根据算法设计，实现了RWF-Tree

- 结构设计：一个 RWF-Tree 在实际存储为
    - 存储元素信息的哈希表使用 redis 原有的 hash set 底层实现。哈希表在有序集合的名字基础上加一个后缀存储，对外不可见。由于哈希表的设计中值只能是字符串，因此将自定数据结构指针值当作字符串存入其中。
- 服务于该 Tree 的新结构体及其操作
    * *TreeNode* : RWF-Tree树节点，存储有remove-history以及树节点的各属性值
- 实用函数/宏：
    * *rtnHTGet*，获得元素存储数据结构，不存在则依据参数是否创建。
    * *subTree*，返回子树节点的集合。