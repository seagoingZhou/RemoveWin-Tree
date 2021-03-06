import os
import csv
import sys
#from zss import simple_distance, Node
#import networkx as nx


class TreeNode(object):
    def __init__(self,id,value):
        super().__init__()
        self.id = id
        self.value = value

def tree_gen(raw):
    raw_map = {}
    raw_map['0,0'] = TreeNode('0,0','root')
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
        tree_map[data[0]] = sorted(tree_map[data[0]],key = lambda x:x.id)
    ted_tree = {}
    ted_tree['0,0'] = Node('root',[])
    for data in raw:
        ted_tree[data[0]] = Node(data[1],[])
    for child in tree_map['0,0']:
        ted_tree['0,0'].addkid(ted_tree[child.id])
    for data in raw:
        for child in tree_map[data[0]]:
            ted_tree[data[0]].addkid(ted_tree[child.id])

    return ted_tree['0,0']

def n_match(node1, node2):
    key1 = node1['name']
    key2 = node2['name']
    return key1 == key2
'''
def graph_gen(raw):
    raw_map = {}
    raw_map["0,0"] = "root"
    for data in raw:
        raw_map[data[0]] = data[1]
    tree_map = {}
    tree_map["0,0"] = []
    for data in raw:
        tree_map[data[0]] = []
        tree_map[data[2]] = []
    for data in raw:
        tree_map[data[2]].append(data[0])
    G = nx.DiGraph()
    list = ["0,0"]
    G.add_node("0,0", name = "root")
    while list:
        cur = list.pop(0)
        for child in tree_map[cur]:
            c_name = raw_map[child]
            G.add_node(child, name = c_name)
            G.add_edge(cur, child)
            list.append(child)
    print("graph gen done\n")
    return G
'''
def jac_gen(raw):
    ret = {}
    ret['0,0'] = ['root', 'null']
    for data in raw:
        if len(data) == 3:
            ret[data[0]] = [data[1], data[2]]
        else:
            print("invalid data")
            print(data)
    return ret

def jaccord(d0, d1):
    cnt = 0
    grd = 0
    for key in d0:
        if key in d1:
            cnt += 1
            grd += 1 + int(d0[key][0]==d1[key][0]) + int(d0[key][1]==d1[key][1])
    return str(cnt) + '#' + str(grd)
'''
def ted_read(directory="."):
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
            t1 = tree_gen(tmp1)
            t2 = tree_gen(tmp2)
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

def ged_read(directory="."):
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
            g1 = graph_gen(tmp1)
            g2 = graph_gen(tmp2)
            ged = nx.graph_edit_distance(g1, g2, node_match = n_match)
            len1 = len(tmp1)
            len2 = len(tmp2)
            print([ged,len1,len2])
            data.append([ged,len1,len2])
        else:
            if flag==1:
                tmp1.append(line.split())
            elif flag==2:
                tmp2.append(line.split())
    return data
'''
def read(directory=".", gen_fun = jac_gen, cmp_fun = jaccord):
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
            t1 = gen_fun(tmp1)
            t2 = gen_fun(tmp2)
            ted = cmp_fun(t1, t2)
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


def save(save_dir, foldname, data):
    filename = "{dir}/{name}.csv".format(dir= save_dir,name = foldname)
    with open(filename, 'w') as f:
        csvwriter = csv.writer(f)
        csvwriter.writerows(data)


REPLICA = 1
DELAY = 3
SPEED = 2
MODEL_MAP = {REPLICA:"replica", SPEED:"speed", DELAY:"delay"}

MODEL = REPLICA;
raw_dir1 = "D:/data/tree/raw-data/{model}/addrem".format(model = MODEL_MAP[MODEL])
raw_dir2 = "D:/data/tree/raw-data/{model}/chg".format(model = MODEL_MAP[MODEL])
save_dir1 = "D:/data/tree/processed-data/{model}/addrem".format(model = MODEL_MAP[MODEL])
save_dir2 = "D:/data/tree/processed-data/{model}/chg".format(model = MODEL_MAP[MODEL])

def pre_process(raw_dir, save_dir):
    foldlist = os.listdir(raw_dir)
    for fold in foldlist:
        d = "{dir}/{foldname}".format(dir=raw_dir,foldname=fold)
        print(d);
        data = read(d, gen_fun = jac_gen, cmp_fun = jaccord)
        save(save_dir, fold, data)

pre_process(raw_dir1,save_dir1)
pre_process(raw_dir2,save_dir2)



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


