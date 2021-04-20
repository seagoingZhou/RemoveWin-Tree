#  RWF 设计框架的应用与拓展

RWF 设计框架是一个最近提出的CRDT设计框架。该框架能够指导无冲突复制数据容器类型( RWF-DT )的设计，并基于Redis数据库，提供了 RWF-DT 实现平台。RWF 设计框架项目的细节详见[RWF](https://github.com/elem-azar-unis/CRDT-Redis/)

## RWF 设计框架的挑战

RWF 设计框架的应用与发展需要应对下面几个要素的挑战。

* RWF-DT命令操作的扩展
    * 已有的RWF-DT研究重点在于数据类型实现无冲突复制的原理，其设计与实现通常只考虑数据类型的核心命令操作。**过于简单的操作命令**限制了 RWF-DT 的应用场景

* 更多 RWF-DT 数据类型的设计实现
    * 基于RWF设计框架的**RWF-DT种类较少**，不足以展示出RWF设计框架的通用性和泛用性的特点。

## 主要工作

本项目的主要工作如下所示：
* RWF-Set
    * 我们以 RWF 设计框架的核心 RWF-Set 为研究对象，设计实现了 RWF-Set 二元运算(**交集运算**，**并集运算**和**差集运算**)的算法。该二元运算算法设计**原理**具有通用性，适用于其他无冲突复制Set数据类型。基于该二元运算算法设计原理，我们设计实现了Observed-Remove Set (OR-Set) 和 Positive-Negative Set (PN-Set) 的二元运算算法。同时设计实验，对比测试了三种无冲突复制Set数据类型的性能。
* RWF-Tree
    * 我们提出了 RWF-Tree 的设计，实现并度量了 RWF-Tree 的性能表现

本项目主要工作的细节详见论文。

## 项目仓库结构
仓库主要由**redis-4.0.8**和**experiment**两个文件夹组成，各文件夹的主要内容如下表所示

|	文件夹名称		|	概要	|	
| ------------- | --------------------- | --------------------- |
| **redis-4.0.8**	|	项目的核心实现。RWF 设计框架基于Redis提供了 RWF-DT 实现平台，我们在该平台上实现了三种无冲突复制Set数据类型和 RWF-Tree ，以及对应的命令操作。具体细节详见[无冲突复制Set数据类型实现](./redis-4.0.8/README.md)	| 
| **experiment**	|	实验测试程序。三种无冲突复制Set数据类型和 RWF-Tree 的性能测试程序。具体细节详见[实验测试](./experiment/README.md)		| 
