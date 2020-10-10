#include "generator/generator.h"
#include "generator/uniform_generator.h"
#include "generator/zipfian_generator.h"
#include "generator/core_generator.h"
#include "uid.h"
#include "tree.h"
#include "conflict.h"
#include "measure.h"

#include <hiredis/hiredis.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#define MAX 100000

using namespace ycsbc;
using namespace std;
using std::string;

int main(){
/*   
    redisReply *reply1;
    redisReply *reply2;
    redisReply *reply3;
    redisReply *reply4;
    redisReply *reply5;

    redisContext *c1;
    redisContext *c2;
    redisContext *c3;
    redisContext *c4;
    redisContext *c5;
    c1 = redisConnect("192.168.63.132", 6379);
    c2 = redisConnect("192.168.63.132", 6380);
    c3 = redisConnect("192.168.63.132", 6381);
    c4 = redisConnect("192.168.63.132", 6382);
    c5 = redisConnect("192.168.63.132", 6383);
    reply1 = (redisReply *) redisCommand(c1,"treemembers t");
    reply2 = (redisReply *) redisCommand(c2,"treemembers t");
    reply3 = (redisReply *) redisCommand(c3,"treemembers t");
    reply4 = (redisReply *) redisCommand(c4,"treemembers t");
    reply5 = (redisReply *) redisCommand(c5,"treemembers t");
    
    
    //处理redis reply
    Tree *t_79 = parserTree(reply1);
    Tree *t_80 = parserTree(reply2);
    Tree *t_81 = parserTree(reply3);
    Tree *t_82 = parserTree(reply4);
    Tree *t_83 = parserTree(reply5);
    cout<<"**************6379******************"<<endl;
    for (auto &it:t_79->node){
        cout<<it<<"\t"<<t_79->tree[it]->value<<"\t"<<t_79->tree[it]->parent<<endl;
    }
    cout<<"************************************"<<endl;
    cout<<"**************6380******************"<<endl;
    for (auto &it:t_80->node){
        cout<<it<<"\t"<<t_80->tree[it]->value<<"\t"<<t_80->tree[it]->parent<<endl;
    }
    cout<<"************************************"<<endl;
    
    
    



    int cnt1 = isSameTree(t_79,t_83);
    int cnt2 = isSameTree(t_79,t_80);
    int cnt3 = isSameTree(t_79,t_81);
    int cnt4 = isSameTree(t_79,t_82);
    //int cnt5 = isSameTree(t,t_83);
    

    cout<<"一致性: "<<cnt1<<","<<cnt2<<","<<cnt3<<","<<cnt4<<endl;
    freeReplyObject(reply1);
    freeReplyObject(reply2);
    freeReplyObject(reply3);
    freeReplyObject(reply4);
    freeReplyObject(reply5);
    redisFree(c1);
    redisFree(c2);
    redisFree(c3);
    redisFree(c4);
    redisFree(c5);
    delete t_79;
    delete t_80;
    delete t_81;
    delete t_82;
    delete t_83;
*/

/*
    CoreGenerator *gen = new CoreGenerator(0,6,0,MAX);
    Tree *t = new Tree(4);
    Uid *uid = new Uid();

    //load tree
    cout<<"******data loading******"<<endl;

    for (int i=0; i<30; i++){
        
        string pid = t->getInsertParentId();
        string id = uid->Next();
        string value = gen->buildKeyName();
        t->insert(pid,id,value);
        cout<<"insert "<<id<<":"<<value<<" to "<<pid<<endl;
        
    }
    
    
    cout<<"******load completed******"<<endl;

    string leafn = t->getNonLeafId();
    cout<<leafn<<endl;
    vector<string> Ancestor = t->ancestors(leafn);
    vector<string> decentors = t->subtree(leafn);
    for (auto &it:Ancestor){
        cout<<it<<" ";
    }
    cout<<endl;
    for (auto &it:decentors){
        cout<<it<<" ";
    }
    cout<<endl;

*/    
    redisReply *reply1;
    redisContext *c1;
    c1 = redisConnect("192.168.63.132", 6379);
    reply1 = (redisReply *) redisCommand(c1,"ping");
    if (!strcmp(reply1->str,"PONG")){
        printf("PING:%s\n",reply1->str);
    }
    
    //cout<<reply1->str<<endl;
    

    

    freeReplyObject(reply1);
    redisFree(c1);

    return 0;
}