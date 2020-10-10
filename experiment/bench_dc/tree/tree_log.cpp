#include "tree_log.h"
#include <string.h>
#include <string>
#include <iostream>

using namespace std;


vector<string> tree_log::subtree(string pid)
{
    //lock_guard<mutex> lk(mtx);
    vector<string> ret;
    if (map.find(pid)!=map.end()){
        
        queue<string> que;
        que.push(pid);
        while(!que.empty()){
            string id = que.front();
            que.pop();
            vector<string> v = map[id]->children;
            for (auto it=v.begin();it!=v.end();++it){
                que.push(*it);
            }
            ret.push_back(id);
        }

        return ret;

    }

}

vector<string> tree_log::subtree_with_mutex(string pid){
    lock_guard<mutex> lk(mtx);
    return subtree(pid);
}

void tree_log::insert(string pid,string uid,string value)
{
    lock_guard<mutex> lk(mtx);
    if (map.find(uid)==map.end()
        &&map.find(pid)!=map.end()){

        nodes.push_back(uid);
        treenode* tn = new treenode;
        tn->parent = pid;
        tn->value = value;
        map.insert(pair<string,treenode*>(uid,tn));
        map[pid]->children.push_back(uid);
    }

}

void tree_log::del(string pid)
{
    lock_guard<mutex> lk(mtx);
    if (map.find(pid)!=map.end()){
        vector<string> subtreeEles = subtree(pid);

        string parent = map[pid]->parent;
        auto search = map.find(parent);
        if (search != map.end()){
            auto iter = find(map[parent]->children.begin(),map[parent]->children.end(),pid);
            if(iter!=map[parent]->children.end()){
                map[parent]->children.erase(iter);
            } 
        }


        for (auto &it : subtreeEles){
            //删除子节点信息
            delete map[it];
            unordered_map<string,treenode*>::iterator key = map.find(it);
            if (key!= map.end()){
                map.erase(key);
            }
            auto iter = find(nodes.begin(),nodes.end(),it);
            if (iter!=nodes.end()){
                nodes.erase(iter);
        }
    }

    }
}

void tree_log::changevalue(string uid,string value)
{
    lock_guard<mutex> lk(mtx);
    if (map.find(uid)!=map.end()){
        map[uid]->value = value;
    }


}

void tree_log::members(redisReply *reply){
    vector<string> t_read;
    vector<string> t_actural;

    {
        lock_guard<mutex> lk(mtx);
        vector<string> tnodes = subtree("0,0");
        for (string id:tnodes){
            if (id.compare(string("0,0"))==0) continue;
            string aele = id +" "+ 
                    string(map[id]->value)+" "+
                    string(map[id]->parent);
            t_actural.push_back(aele);
        }

        if (reply->type == REDIS_REPLY_ARRAY) {
            for (int i = 0; i < reply->elements; i++) {
                
                char id[32];
                char name[32];
                char parent[32];
                int uidPos = 0;
                int namePos = 0;
                int parentPos = 0;

                char* p = reply->element[i]->str;
                while(*p != ' '){
                    id[uidPos++]=*p;
                    p++;
                }
                id[uidPos] = '\0';

                p++;
                while(*p != ' '){
                    name[namePos++] = *p;
                    p++;
                    
                }
                name[namePos] = '\0';

                p++;
                while(*p != ' '){
                    parent[parentPos++] = *p;
                    p++;
                    
                }
                parent[parentPos] = '\0';

                if (!strcmp(id,"0,0")) continue;
                string ele = string(id)+" "+string(name)+" "+string(parent);
                t_read.push_back(ele);
            }
        }
    }

    {
        lock_guard<mutex> lk(m_mtx);
        tLog.emplace_back(t_read,t_actural);
    }
    
    
}

string tree_log::random_get()
{
    lock_guard<mutex> lk(mtx);
    if (nodes.size() == 1){
        return nodes[0];
    }

    int idx;
    string pid;
    vector<string> v;
    do{
        idx = intRand(nodes.size());
        pid = nodes[idx];
        v = map[pid]->children;
    }while (pid == "0,0");
    
    return pid;
}

string tree_log::random_delete_get(){
    lock_guard<mutex> lk(mtx);
    int idx;
    string pid;
    vector<string> v;
    do{
        idx = intRand(nodes.size());
        pid = nodes[idx];
        v = subtree(pid);
    }while (v.size() > 15||v.size()<3);
    
    return pid;
}

string tree_log::random_insert_get(){
    lock_guard<mutex> lk(mtx);
    if (nodes.size() == 1){
        return nodes[0];
    }
    int idx;
    string pid;
    vector<string> v;
    do{
        idx = intRand(nodes.size());
        pid = nodes[idx];
        v = map[pid]->children;
    }while (v.size() > 5);
    
    return pid;
}

void tree_log::write_file(){
    char n[64], f[64];
    sprintf(n, "%s/%s:%d,%d,(%d,%d)", dir, type, TOTAL_SERVERS, OP_PER_SEC, DELAY, DELAY_LOW);
    bench_mkdir(n);

    sprintf(f, "%s/s.tree", n);
    FILE *tree = fopen(f, "w");
    for (auto t:tLog){
        fprintf(tree,"*\n");
        //写入server上的tree
        for (string str:t.t_read){
            fprintf(tree,"%s\n",str.c_str());
        }

        fprintf(tree,"--\n");
        
        //写入测试程序中的tree
        for (string str:t.t_actural){
            fprintf(tree,"%s\n",str.c_str());
        }
        fprintf(tree,"**\n");
        
    }

    fflush(tree);
    fclose(tree);
}

int tree_log::tSize(){
    lock_guard<mutex> lk(mtx);
    return nodes.size();
}