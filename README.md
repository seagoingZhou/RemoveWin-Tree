#  Remove-Win Framework Research

## 基于RWF设计框架的CRDT

* Set
    * Observed-Remove Set (Add-Win Set)
    * Positive-Negative Set
    * RWF-Set
* Tree
    * RWF-Tree

## 编译

本项目基于Redis实现。首先需要在文件夹*redis-4.0.8*编译项目

```bash
cd redis-4.0.8
make
```

## 运行

为了避免环境变量和配置的影响，以及统一路径名，本项目在Docker环境运行。Docker通过*docker*文件夹里的脚本文件配置、启动和关闭。
```bash
sh ./start.sh
```

Docker容器启动后，根据Docker容器名进入容器
```bash
sudo docker exec -it redis0 /bin/bash  
```

文件夹*experiment/redis_test*内的脚本文件用于CRDT数据类型的使用。默认的对等复制集群有5个Redis实例，其端口号从6379到6383。脚本文件的功能如下所示：

* **server.sh [parameters]** 启动Redis服务器。默认启动5个Redis服务器实例，通过参数可启动自定义Redis服务器
* **construct_replication.sh [parameters]** 建立对等复制网络。默认在端口号为6379到6383的5个Redis服务器实例之间建立对等复制网络，用户可自定义Redis服务器实例之间的对等连接。
* **client.sh <server_port>** 启动Redis客户端，并连接对应端口号的Redis服务器
* **shutdown.sh [parameters]** 关闭Redis实例
* **clean.sh [parameters]** 清除所有的数据库文件(.rdb文件)和日志文件(.log文件)

下面是一个运行demo

首先终端进入路径*experiment/redis_test*，启动Redis服务器并建立对等复制模式。
```bash
cd  /Redis/RWTree/experiment/redis_test
./server.sh
./construct_replication.sh
```

此时Redis服务器实例已启动，对等复制模式已建立，用户可以通过Redis客户端进行操作。
```bash
./client.sh <server_port>
```

当操作结束时，用户可以关闭Redis服务器实例。
```bash
./shutdown.sh
```
最后清除所有的数据库文件(.rdb文件)和日志文件(.log文件)。

```bash
./clean.sh
```

## CRDT操作命令

相同类型CRDT的不同实现为用户提供了相同的操作。我们使用 **[type][op]** 命令操作命令


### Set
不同Set的前缀 **[type]** 如下所示:

* Observed-Remove Set : **OR**
* Positive-Negative Set : **PN**
* RWF-Set : **RWF**

Set的更新操作命令如下所示

* **[type]SADD key member [member …]** : 将一个或多个 *member*元素加入到集合*key*当中，已经存在于集合的*member*元素将被忽略。
* **[type]SREM key member [member …]** : 移除集合*key*中的一个或多个*member*元素，不存在的*member*元素会被忽略。
* **[type]SINTERSTORE destination key [key …]** : 计算给定集合的交集，将结果保存到*destination*集合，
* **[type]SUNIONSTORE destination key [key …]** : 计算给定集合的并集，将结果保存到*destination*集合，
* **[type]SDIFFSTORE destination key [key …]** : 计算给定集合的差集，将结果保存到*destination*集合，

### Tree

RWF-Tree的操作命令如下所示：
* **RWFTREECREATE tree\_name** : 创建一个名为*tree\_name*的RWF-Tree。
* **RWFTREEINSERT  tree\_name value parent\_id id** : 向RWF-Tree *tree\_name*中id为*parent\_id*的节点插入一个子节点，该子节点的id为*id*，value为*value*
* **RWFTREEDELETE tree\_name id** 删除RWF-Tree *tree\_name*中id为*id*的节点及其所有子节点
* **RWFTREECHANGEVALUE tree\_name id value** 将RWF-Tree *tree\_name*中id为*id*的节点的value值修改为*value*
* **RWFTREEMOVE tree\_name destination\_id  source\_id** 移动RWF-Tree *tree\_name*中id为*source\_id*的节点及其所有子节点到id为*destination\_id*的节点下，使得节点*destination\_id*为节点*source\_id*的父节点
* **TREEMEMBERS tree\_name** 返回RWF-Tree *tree\_name*中所有成员及其属性信息
