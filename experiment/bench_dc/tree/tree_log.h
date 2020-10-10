#ifndef TREE_LOG_H
#define TREE_LOG_H

#include <vector>
#include <unordered_map>
#include <string>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <algorithm>
#include <mutex>
#include "../util.h"

using namespace std;

class tree_log : public rdt_log
{
private:
    
    struct treenode
    {
        string value;
        string parent;
        vector<string> children;

        //treenode(string v,string p,vector<string> ch)
            //:value(v),parent(p),children(ch) {}

    };

    struct t_log
    {
        vector<string> t_read;
        vector<string> t_actural;

        t_log(vector<string> r,vector<string> a)
            :t_read(r),t_actural(a) {}
    };

    vector<string> nodes;
    unordered_map<string,treenode*> map;

    mutex mtx,m_mtx;
    vector<t_log> tLog;

public:
    tree_log(const char *type, const char *dir) : rdt_log(type, dir) {
        string id = "0,0";
        nodes.push_back(id);

        treenode* tn = new treenode;
        tn->parent = "null";
        tn->value = "_root_";
        map.insert(pair<string,treenode*>(id,tn));
    }
    ~tree_log(){
        for(auto it=map.begin();it!=map.end();it++){
            delete it->second;
        }
    }

    void insert(string pid,string uid,string value);
    void del(string pid);
    void changevalue(string uid,string value);
    void members(redisReply *reply);
    void write_file();

    string random_get();
    string random_delete_get();
    string random_insert_get();
    vector<string> subtree(string uid);
    vector<string> subtree_with_mutex(string uid);
    int tSize();




};


#endif