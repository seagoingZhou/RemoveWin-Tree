from zss import simple_distance, Node

# Node(label, children)
# a---> b --> d
#  \--> c
L = []
L.append(Node('d',[]))
L.append(Node('c',[]))
L.append(Node('b',[]))
L[2].addkid(L[0])
L.append(Node('a', [L[2],L[1]]))


i3 = Node('d',[])
i2 = Node('c', [])
i1 = Node('b', [])
i0 = Node('a', [])
i1.addkid(i3)
i0.addkid(i1)
#i0.addkid(i2)
#i0.addkid(i3)

# a---> b
#  \--> d
j2 = Node('d',[])
j1 = Node('b', [])
j0 = Node('a', [j1,j2])
j0.addkid(Node('c',[]))
#assert simple_distance(a, a2) == 1
print(simple_distance(i0, L[3]))