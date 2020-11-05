from zss import simple_distance, Node
class TreeNode(object):
    def __init__(self,id,value):
        super().__init__()
        self.id = id
        self.value = value

A = [['#0','a','0,0'],['#1','b','#0'],['#2','c','#0'],['#4','e','#2'],['#5','f','#2'],['#3','d','#2']]
B = [['#0','a','0,0'],['#1','b','#0'],['#2','c','#0'],['#3','d','#2'],['#4','e','#2']]
C = [['#0','a','0,0'],['#1','b','#0'],['#2','c','#0']]

i3 = Node('c',[])
i2 = Node('b',[])
i1 = Node('a',[i2,i3])
i0 = Node('root',[i1])

'''
Aa = {}
Aa['0,0'] = TreeNode('0,0','root')
for tn in A:
    Aa[tn[0]] = TreeNode(tn[0],tn[1])

At = {}
At['0,0'] = []
for tn in A:
    At[tn[0]] = []

for tn in A:
    At[tn[2]].append(Aa[tn[0]])

for tn in A:
    At[tn[0]] = sorted(At[tn[0]],key = lambda x:x.value)
'''

def tedTreeGen(raw):
    raw_map = {}
    raw_map[0,0] = []
    for data in raw:
        raw_map[data[0]] = TreeNode(data[0],data[1])
    tree_map = {}
    tree_map['0,0'] = []
    for data in raw:
        tree_map[data[0]] = []
    for data in raw:
        tree_map[data[2]].append(raw_map[data[0]])
    for data in raw:
        tree_map[data[0]] = sorted(tree_map[data[0]],key = lambda x:x.value)
    tedTree = {}
    tedTree['0,0'] = Node('root',[])
    for data in raw:
        tedTree[data[0]] = Node(data[1],[])
    for child in tree_map['0,0']:
        tedTree['0,0'].addkid(tedTree[child.id])
    for data in raw:
        for child in tree_map[data[0]]:
            tedTree[data[0]].addkid(tedTree[child.id])

    return tedTree['0,0']

At = tedTreeGen(A)
Bt = tedTreeGen(B)
Ct = tedTreeGen(C)
print(simple_distance(Ct, i0))
print(simple_distance(At, Bt))
'''
for tn in A:
    output = tn[0]+':'+tn[1]+' ['
    for child in At[tn[0]]:
        output = output+child.id+':'+child.value+' '
    print(output+']')
'''





