from zss import simple_distance, Node

# Node(label, children)
# a---> b
#  \--> c
i2 = Node('c1', [])
i1 = Node('b', [])
i0 = Node('a', [i1, i2])
#assert simple_distance(a, a) == 0
print(simple_distance(i0, i1))

# a---> c
j1 = Node('c1', [])
j0 = Node('a', [j1])
#assert simple_distance(a, a2) == 1
print(simple_distance(j0, i0))