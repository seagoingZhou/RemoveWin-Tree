#ifndef _TREE_H_
#define _TREE_H_

#include <vector>
#include <map>
#include <string>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <algorithm>
#include <random>
#include <thread>

using namespace std;

typedef struct treenode
{
    string value;
    string parent;
    vector<string> children;

}treenode;


class Tree
{
private:
    int leafnum;
    
public:
    vector<string> node;
    vector<string> leaf;
    map<string,treenode*> tree;
   // map<string,string> uidToName;

    Tree(int load);
    ~Tree();

    string getParentId();
    string getInsertParentId();
    string getDeleteId();
    string getLeafId();
    void insert(string pid,string uid,string value);
    void del(string pid);
    void changevalue(string uid,string value);
    void move(string dst_id,string src_id);
    vector<string> subtree(string uid);
    vector<string> ancestors(string uid);

    
};

Tree::Tree(int load)
{
    leafnum = load;
    string id = "0,0";
    node.push_back(id);

    treenode* tn = new treenode;
    tn->parent = "null";
    tn->value = "_root_";
    tree.insert(pair<string,treenode*>(id,tn));
}

Tree::~Tree(){
    for (auto it=tree.begin(); it!=tree.end();++it){
        delete it->second;
    }
}

static int intRand(const int max){
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_int_distribution<int> distribution(0,max-1);
    return distribution(*rand_gen);
}



void Tree::insert(string pid,string uid,string value){
    node.push_back(uid);
    treenode* tn = new treenode;
    tn->parent = pid;
    tn->value = value;
    tree.insert(pair<string,treenode*>(uid,tn));
    tree[pid]->children.push_back(uid);

    auto iter = find(leaf.begin(),leaf.end(),pid);
    if (iter!=leaf.end()){
        leaf.erase(iter);
    }
    leaf.push_back(uid);
}

string Tree::getParentId(){

    if (node.size() == 1){
        return node[0];
    }

    int idx;
    string pid;
    vector<string> v;
    do{
        idx = intRand(node.size());
        pid = node[idx];
        v = tree[pid]->children;
    }while (pid == "0,0");
    
    return pid;
}

string Tree::getInsertParentId(){

    if (node.size() == 1){
        return node[0];
    }

    int idx;
    string pid;
    vector<string> v;
    do{
        idx = intRand(node.size());
        pid = node[idx];
        v = tree[pid]->children;
    }while (v.size() >= leafnum || pid == "0,0");
    return pid;

}


string Tree::getDeleteId(){
    string ret;
    vector<string> v;
    do{
        ret = getParentId();
        v = subtree(ret);
        //v = tree[ret]->children;
    }while(v.size()>8);

    return ret;
}

string Tree::getLeafId(){
    return leaf[0];
}

void Tree::changevalue(string uid,string value){
    tree[uid]->value = value;
}

vector<string> Tree::subtree(string uid){
    vector<string> ret;
    queue<string> que;
    que.push(uid);
    while(!que.empty()){
        string id = que.front();
        que.pop();
        vector<string> v = tree[id]->children;
        for (auto it=v.begin();it!=v.end();++it){
            que.push(*it);
        }
        ret.push_back(id);
    }

    return ret;
}

vector<string> Tree::ancestors(string uid){
    vector<string> ret;
    string id = uid;
    while(id != "0,0"){
        
        id = tree[id]->parent;
        ret.push_back(id);
    }
    return ret;
}

void Tree::del(string uid){
    auto iter0 = find(node.begin(),node.end(),uid);
    if (iter0==node.end()) return;
    vector<string> subtreeEles = subtree(uid);

    string parent = tree[uid]->parent;
    auto search = tree.find(parent);
    if (search != tree.end()){
        auto iter = find(tree[parent]->children.begin(),tree[parent]->children.end(),uid);
        if(iter!=tree[parent]->children.end()){
            tree[parent]->children.erase(iter);
        } 
    }

    if ((tree[parent]->children).size()==0){
        leaf.push_back(parent);
    }

    auto iter = find(leaf.begin(),leaf.end(),uid);
    if (iter!=leaf.end()){
            leaf.erase(iter);
        }

    for (auto &it : subtreeEles){
        //删除子节点信息
        delete tree[it];
        map<string,treenode*>::iterator key = tree.find(it);
        if (key!= tree.end()){
            tree.erase(key);
        }
        auto iter = find(node.begin(),node.end(),it);
        if (iter!=node.end()){
            node.erase(iter);
        }
    }
}

void Tree::move(string dst_id,string src_id){
    string src_parent = tree[src_id]->parent;
    auto iter = find(tree[src_parent]->children.begin(),
                    tree[src_parent]->children.end(),src_id);
    if (iter!=tree[src_parent]->children.end()){
            tree[src_parent]->children.erase(iter);
    }

    tree[dst_id]->children.push_back(src_id);
    tree[src_id]->parent = dst_id;

}

#endif