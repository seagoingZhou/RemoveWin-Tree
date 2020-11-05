import os
import csv
import sys
from zss import simple_distance, Node


class TreeNode(object):
    def __init__(self,id,value):
        super().__init__()
        self.id = id
        self.value = value

def tedTreeGen(raw):
    raw_map = {}
    raw_map[0,0] = []
    for data in raw:
        raw_map[data[0]] = TreeNode(data[0],data[1])
    tree_map = {}
    tree_map['0,0'] = []
    for data in raw:
        tree_map[data[0]] = []
        tree_map[data[2]] = []
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

def read(directory="."):
    tmp1 = []
    tmp2 = []
    flag = 0;
    data = []
    for line in open(directory + "/s.tree"):
        ss=line.strip()
        if ss == '*':
            tmp1 = []
            flag = 1
        elif ss == '--':
            tmp2 = []
            flag = 2
        elif ss =='**':
            t1 = tedTreeGen(tmp1)
            t2 = tedTreeGen(tmp2)
            ted = simple_distance(t1, t2)
            len1 = len(tmp1)
            len2 = len(tmp2)
            print([ted,len1,len2])
            data.append([ted,len1,len2])
        else:
            if flag==1:
                tmp1.append(line.split())
            elif flag==2:
                tmp2.append(line.split())
    return data


def tedSave(foldname):
    filename = "{name}.csv".format(name = foldname)
    with open(filename, 'w') as f:
        csvwriter = csv.writer(f)
        csvwriter.writerows(data)


foldlist = os.listdir("./RawData")
for fold in foldlist:
    d = "{dir}/{foldname}".format(dir='./RawData',foldname=fold)
    print(d);
    data = read(d)
    tedSave(fold)



'''

def testread(ztype, server, op, delay, low_delay, directory='.'):
    d = "{dir}/{t}:{s},{o},({d},{ld})".format(dir=directory, t=ztype, s=server, o=op, d=delay, ld=low_delay)
    file = open(d + "/s1.tree")
    for line in file.readlines():
        #line.strip()
        ss = line.strip()
        if ss=='*':
            print('1 '+line)
        elif ss=='**':
            print('2 '+line)
        elif ss=='***':
            print('3 '+line)
'''
'''
def main(argv):
    ztype = 'rw'
    server = 9
    op = 100000
    delay = 50
    low_delay = 10
    directory = './ardominant'
    if len(argv) == 4:
        server = argv[0]
        op = argv[1]
        delay = argv[2]
        low_delay = argv[3]

    data = read(ztype, server, op, delay, low_delay, directory)
    print(data)
    tedSave(data,ztype, server, op, delay, low_delay, directory)


if __name__ == "__main__":
    main(sys.argv[1:])
'''        


