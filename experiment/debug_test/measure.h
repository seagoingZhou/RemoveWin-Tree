#ifndef _MEASURE_H_
#define _MEASURE_H_
#include <vector>
#include <string>
#include <string.h>
#include <algorithm>
#include <hiredis/hiredis.h>
#include "tree.h"

using namespace std;

int measureVectors(vector<string> v1,vector<string> v2){

    vector<string> src = v1.size()<v2.size()?v1:v2;
    vector<string> dst = v1.size()<v2.size()?v2:v1;
    int interCnt = 0;
    for (auto &it : src){
        bool flag = find(dst.begin(),dst.end(),it)==dst.end()?
                            false:true;
        if (flag){
            interCnt++;
        }

    }

    return src.size()+dst.size()-2*interCnt;
}

int isSameTree(Tree *t1, Tree *t2){
    int ret = 1;

    if (t1->node.size()!=t2->node.size()){
        ret = false;
    } else {
        for (auto &it : t1->node){
            
            string p1 = t1->tree[it]->parent;
            string p2 = t2->tree[it]->parent;

            string val1 = t1->tree[it]->parent;
            string val2 = t2->tree[it]->parent;
            if (val1 != val2 || p1 != p2){
                ret = 0;
                break;
            }
        }
    }
    

    return ret;

}

Tree * parserTree(redisReply *reply){
    Tree *t = new Tree(5);
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
            t->node.push_back(id);
            treenode* tn = new treenode;
            tn->parent = parent;
            tn->value = name;
            t->tree.insert(pair<string,treenode*>(id,tn));
            
        }
    }

    return t;
}

#endif