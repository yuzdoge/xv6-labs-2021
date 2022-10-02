Q: Why are there missing keys with 2 threads, but not with 1 thread? Identify a sequence of events with 2 threads that can lead to a key being missing.

A: 一个例子：假设T1和T2都要对table[1]插入一个节点，T1先让table[1]指向该新节点，而T2后被调度，随后T2也让table[1]指向属于它的新节点，那么T1创建的新节点则变成了孤儿节点，从而丢失。
