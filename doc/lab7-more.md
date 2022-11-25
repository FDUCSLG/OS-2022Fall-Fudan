# Lab 7 补充说明

可参考lab7.pdf给出的结构图，我们将container组织为一棵树，每一个container都有独立的**调度队列、pid分配器和root proc**。container会被加入其上级container的调度队列中，与进程一样参与调度，只在schinfo中通过flag加以区分。当container被调度到时，它在它的调度队列中重复调度操作。root\_container是container树的根节点，原先系统中全局的root\_proc被设置为root container的root proc，原先系统中全局的调度队列被设置为root container的调度队列，但原先全局的pid分配器保留，所分配的global pid，与container（包括root container）分配的local pid独立。

lab7需要完成的任务以本文档中列出为准

* 我们已经在`struct proc`中添加了`localpid`和`container`，在container.c中定义和初始化了`root_container`。你需要在`init_proc`中将进程初始的`container`设为`root_container`。你还需要补全container.h中的`struct container`，添加pid分配器相关的数据，在container.c的`init_container`中添加初始化pid分配器相关的代码，在proc.c的`start_proc`中根据进程所属的`container`为其分配local pid，并**返回local pid**。
  
  > `struct proc`中原先的pid称为global pid，global pid不等价于root container的local pid，其分配和回收请维持原样。
  > 
  > 你可能需要在`struct container`中加锁保护你的pid分配器，也可以直接使用global pid分配器的锁。
  > 
  > `start_proc`中规定parent=NULL时设置parent为root\_proc仍指全局的那个root\_proc，无需改动。

* proc.c中的`kill`**使用global pid**，不需要修改

* proc.c中的`exit`需要将子进程移交给root\_proc改为**移交给所属container的root proc**。保证不会出现某个container的root proc退出，建议添加`ASSERT(this != this->container->rootproc && !this->idle);`以方便发现错误。

* proc.c中的`wait`添加了第二个参数pid，需要**将子进程的global pid填充到\*pid中，并返回local pid**。不要忘了回收local pid。

* 我们在schinfo.h中定义了`struct schqueue`，保存调度队列的元数据。请重构你的scheduler，将调度队列的元数据（如链表的head，红黑树的root等）移到schqueue结构体中，并将调度队列的初始化代码移到sched.c中的`init_schqueue`中。

* 在`struct schinfo`中添加flag，以区分schinfo对应的是group（container）还是process，并修改sched.c中的`init_schinfo`，根据第二个参数设置相应的flag。

* **重构你的调度器**。在activate等函数中，你需要使用`proc->container->schqueue`来访问**进程所在的调度队列**，而非原先直接引用全局变量。在`pick_next`中，你需要在`root_container.schqueue`中选取一个schinfo节点，若该节点对应于group（container），**在该container的schqueue中重复pick操作**，若该节点对应于process，调度到该process。当整个系统中没有任何可调度到的进程时，才调度到idle。
  
  > 以上只是一种参考算法，你可以采用其他能保证公平性的等价实现。
  > 
  > 如果一个container的schqueue中没有runnable的进程，则应该跳过该container。你也可以此种container的schinfo节点从上级container的schqueue中移除，但可能需要小心处理相关的逻辑，尤其是移除schinfo节点后导致上级container的schqueue为空引发的级联操作，以及container内的进程被唤醒后，将container的schinfo节点加回去的级联操作（建议自行补充测试）。
  > 
  > sched lock被设计为保护所有调度信息，而非仅保护调度队列，因此**所有schqueue共用sched lock**，不需要修改sched lock相关的代码。

* 完成sched.c中的`activate_group`，该函数将container的schinfo节点加入到其parent的schqueue中。

* 完成container.c中的`create_container`，该函数创建并运行一个container。你需要分配一个`struct container`，将其父container设为当前container，为其创建一个root proc，使用`set_parent_to_this`将root proc的父进程设为当前进程。你可能还需要正确设置新建的container和root proc结构体中的其他内容。设置完成后，使用`start_proc`使root proc从参数指定的函数和参数开始运行，并使用`activate_group`启动container。返回新建的container的指针。

以上提及函数和结构体相关的API Reference均已更新。

lab7需要通过`proc_test`、`user_proc_test`和`container_test`。对于`user_proc_test`，要求与lab4一致，即进程和CPU间的工作量应基本均衡。

`container_test`运行时创建的container和process结构如下所示

```
root_container
 |- proc 16-21
 |- container 0
 |   |- proc 0-3
 |   |- container 2
 |   |   |- proc 8-11
 |   |- container 3
 |       |- proc 12-15
 |- container 1
     |- proc 4-7
```

我们要求在同一个container内，为所有container和process赋予相同的权重，即在理想状态下，均衡的工作量应为

```
proc 0-3 ~3500
proc 4-7 ~5250
proc 8-15 ~875
proc 16-21 ~21000
```

此外，仍然要保证各CPU的工作量基本均衡。

> **关于工作量均衡的补充提示**
> 
> 如果你使用CFS调度算法，你可能需要**正确计算container的vruntime**。
> 
> 如果你使用轮换调度算法，你可能需要考虑到**process在running状态下不会被轮到**，因此process和container被轮到的概率存在差异，并作出一些额外处理。
> 
> 如果你使用其它调度算法，请自行思考相关的问题。
> 
> 不正经的思考题：你能否运用《概率论与数理统计》的知识，计算一下不加特殊处理的轮换调度算法会得出的工作量呢？如能与实验结果匹配，虽然是不正经的思考题也可给予少量加分奖励。（<u>量化建模也是OS的一种常用研究手段</u>）

<u>**提交时间延迟至11月20日23:59**</u>


