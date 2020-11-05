from zss import simple_distance, Node
import os
import csv
import sys

A = (
    Node("f")
        .addkid(Node("e"))
        .addkid(Node("f")
            .addkid(Node("h"))
            .addkid(Node("c")
                .addkid(Node("l"))))
    )
B = (
    Node("f")
        .addkid(Node("a")
            .addkid(Node("h"))
            .addkid(Node("c")
                .addkid(Node("l"))))
        .addkid(Node("e"))
    )
ted = simple_distance(A, B) 

print(ted)

foldlist = os.listdir("./RawData")
print(foldlist)