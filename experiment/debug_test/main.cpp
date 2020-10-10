#include "generator/generator.h"
#include "generator/uniform_generator.h"
#include "generator/zipfian_generator.h"
#include "generator/core_generator.h"
#include "uid.h"
#include "tree.h"
#include "conflict.h"
#include "measure.h"

#include <hiredis/hiredis.h>

#include <string>
#include <iostream>
#include <chrono>
#include <thread>

using namespace ycsbc;
using namespace std;
using std::string;

#define MAX 10000

static int intRand(const int min,const int max){
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_int_distribution<int> distribution(min,max);
    return distribution(*rand_gen);
}

void showConstency(Tree *t){

    redisReply *reply1;
    redisReply *reply2;
    redisReply *reply3;
    //redisReply *reply4;
    //redisReply *reply5;

    redisContext *c1;
    redisContext *c2;
    redisContext *c3;
    //redisContext *c4;
    //redisContext *c5;

    c1 = redisConnect("192.168.192.102", 6379);
    c2 = redisConnect("192.168.192.102", 6380);
    c3 = redisConnect("192.168.192.102", 6381);
    //c4 = redisConnect("192.168.192.133", 6382);
    //c5 = redisConnect("192.168.192.133", 6383);
    reply1 = (redisReply *) redisCommand(c1,"treemembers t");
    reply2 = (redisReply *) redisCommand(c2,"treemembers t");
    reply3 = (redisReply *) redisCommand(c3,"treemembers t");
    //reply4 = (redisReply *) redisCommand(c4,"treemembers t");
    //reply5 = (redisReply *) redisCommand(c5,"treemembers t");
    
    //处理redis reply
    Tree *t_79 = parserTree(reply1);
    Tree *t_80 = parserTree(reply2);
    Tree *t_81 = parserTree(reply3);
    //Tree *t_82 = parserTree(reply4);
    //Tree *t_83 = parserTree(reply5);

    int cnt1 = isSameTree(t,t_79);
    int cnt2 = isSameTree(t,t_80);
    int cnt3 = isSameTree(t,t_81);
    //int cnt4 = isSameTree(t,t_82);
    //int cnt5 = isSameTree(t,t_83);
    
    cout<<"一致性: "<<cnt1<<","<<cnt2<<","<<cnt3<<endl;
    freeReplyObject(reply1);
    freeReplyObject(reply2);
    freeReplyObject(reply3);
    //freeReplyObject(reply4);
    //freeReplyObject(reply5);
    redisFree(c1);
    redisFree(c2);
    redisFree(c3);
    //redisFree(c4);
    //redisFree(c5);
    delete t_79;
    delete t_80;
    delete t_81;
    //delete t_82;
    //delete t_83;
}


int main(){

    CoreGenerator *gen = new CoreGenerator(0,6,0,MAX);
    Tree *t = new Tree(4);
    Uid *uid = new Uid();
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

    map<string,string> t_o;
    map<string,string> t_c1;
    map<string,string> t_c2;

    system("python3.6 ../redis_test/connection.py");

    //load tree
    cout<<"******data loading******"<<endl;
    redisReply *reply;
    redisContext *c = redisConnect("192.168.192.100", 6379);
    reply = (redisReply *) redisCommand(c,"treecreat t");
    freeReplyObject(reply);


    for (int i=0; i<100; i++){
        
        string pid = t->getInsertParentId();
        string id = uid->Next();
        string value = gen->buildKeyName();
        t->insert(pid,id,value);
        char cmd[64];
        sprintf(cmd,"treeinsertwithid t %s %s %s",value.c_str(),pid.c_str(),id.c_str());
        reply = (redisReply *) redisCommand(c,cmd);
        freeReplyObject(reply);
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //cout<<"insert "<<id<<":"<<value<<" to "<<pid<<endl;
        printf("%s\n",cmd);
    }
    redisFree(c);
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    cout<<"******load completed******"<<endl;

/*
    c1 = redisConnect("192.168.63.132", 6379);
    c2 = redisConnect("192.168.63.132", 6380);
    for (int i=0;i<40;i++){
        

        
        treeMentain(t,uid,gen,c1,15);
        test_deletechange_conflict(t,uid,gen,c1,c2);
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));   
        
    }
    redisFree(c1);
    redisFree(c2);
    
    string id;
    vector<string> v;
    cout<<"************************************************"<<endl;
    for (int i=0; i<t->node.size(); i++){
        id = t->node[i];
        //cout<<id<<"\t"<<;
        string output = id + " " + t->tree[id]->value;
        v = t->tree[id]->children;
        
        //cout<<"\t"<<t->tree[id]->parent<<"\t";
        output += " "+t->tree[id]->parent+" ";
        string child = "("+to_string(v.size())+ ")[";
        for(auto it=v.begin();it!=v.end();++it){
            child += (" " + *it);
        }
        child += "]";
        output += child;
        cout<<output<<endl;
    }
*/   


    cout<<"*****等待同步**********"<<endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));


    cout<<"*****开始并发操作*****"<<endl;
    c1 = redisConnect("192.168.192.100", 6379);
    c2 = redisConnect("192.168.192.101", 6380);

    for (int i=0; i<50000; i++){
        //if(treeMentain(t,uid,gen,c1,25)){
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //}
        test_changechange_conflict(t,uid,gen,c1,c2);
        
        //test_deletedelete_conflict(t,uid,gen,c1,c2);
        //test_deleteinsert_conflict(t,uid,gen,c1,c2);
        
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    
    redisFree(c1);
    redisFree(c2);
    cout<<"*****并发操作完成*****"<<endl;
    cout<<"*****检测一致性(同步前)*****"<<endl;
    showConstency(t);


    cout<<"*****等待同步**********"<<endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    

    cout<<"*****检测一致性(同步后)*****"<<endl;
    showConstency(t);
    //redisReply *reply1;
    //redisReply *reply2;

    

    delete t;
    return 0;
}


/*
cout<<"************************************************"<<endl;
    string id;
    vector<string> v;
    for (int i=0; i<t->node.size(); i++){
        id = t->node[i];
        //cout<<id<<"\t"<<;
        string output = id + " " + t->tree[id]->value;
        v = t->tree[id]->children;
        
        //cout<<"\t"<<t->tree[id]->parent<<"\t";
        output += " "+t->tree[id]->parent+" ";
        string child = "("+to_string(v.size())+ ")[";
        for(auto it=v.begin();it!=v.end();++it){
            child += (" " + *it);
        }
        child += "]";
        output += child;
        cout<<output<<endl; 
    }
    cout<<"************************************************"<<endl;



*/