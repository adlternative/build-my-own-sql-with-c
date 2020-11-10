# build my own sqlite

#### 这是一个从[db_tutorial](https://github.com/cstack/db_tutorial/)中学习的如何搭建简单sqlite的一个小仓库，将源代码[db.c](https://github.com/cstack/db_tutorial/blob/master/db.c#L236)(仅仅800行)整理成多文件的形式,还可以看到详细的中文注释,希望你能喜欢。

#### get到的知识点

1.　缓存未命中的实现方式

2.　b树的使用(多级页表)

3.　二分的快乐

4.　自动状态机的使用

1. c语言getline的用法

2. cmake的简单使用

####　缺陷

（虽然原作者没有实现完整，如内部节点的分裂和节点的删除，这意味着你无法从这学习到完整的b树操作，本sqlite仅仅实现insert和select去u将固定的内容如`“{id,username,email}”`序列化到文件中。但依旧是c语言和数据结构结合的很好的范例）

如果作者真的懒得去更的话，等我彻底把b树学会了再去写后面的内容！（爬）
另一方面，如果改成用c++在很多方面会方便很多，也易于解耦。


