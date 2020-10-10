#include "generator/generator.h"
#include "generator/uniform_generator.h"
#include "generator/zipfian_generator.h"
#include "generator/core_generator.h"
#include "uid.h"
#include "tree.h"
#include "conflict.h"
#include "config.h"
#include "util.h"

#include <sys/stat.h>
#include <hiredis/hiredis.h>
#include <string>
#include <random>
#include <iostream>
#include <map> 
#include <thread>
#include <chrono>
#include <mutex>
#include <ctime>

using namespace ycsbc;
using namespace std;
using std::string;



void writeTree_local(Tree *t,char* dir){

    FILE *fp = NULL;
    char path[64];
    sprintf(path,"%s/tree.txt",dir);
    fp = fopen(path,"w+");

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
        fprintf(fp,"%s\n",output.c_str());
    }
    
    fclose(fp);
    
    cout<<"************************************************"<<endl;
}

void write_redisReply(char* dir,string ip, int port){

    FILE *fp = NULL;
    char path[64];
    sprintf(path,"%s/%s-%d.txt",dir,ip.c_str(),port);
    fp = fopen(path,"w+");
    redisReply *reply;
    redisContext *c;
    c = redisConnect(ip.c_str(), port);
    reply = (redisReply *) redisCommand(c,"treemembers t");

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (int i = 0; i < reply->elements; i++) {
            char* output = reply->element[i]->str;
            fprintf(fp,"%s\n",output);
            
        }
    }
    fclose(fp);
}

int main(){
 
/*
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator;
    generator.seed(seed);
    std::uniform_real_distribution<double> uniform(0, 1);
*/
    CoreGenerator *gen = new CoreGenerator(0,6,0,MAX);
    Tree *t = new Tree(4);
    Uid *uid = new Uid();
    //char* ip = "192.168.63.132";

    



    //load tree
    cout<<"******data loading******"<<endl;
    redisReply *reply;
    redisContext *c = redisConnect("192.168.192.133", 6379);
    reply = (redisReply *) redisCommand(c,"treecreat t");
    freeReplyObject(reply);

    for (int i=0; i<30; i++){
        
        string pid = t->getInsertParentId();
        string id = uid->Next();
        string value = gen->buildKeyName();
        t->insert(pid,id,value);
        char cmd[64];
        sprintf(cmd,"treeinsertwithid t %s %s %s",value.c_str(),pid.c_str(),id.c_str());
        reply = (redisReply *) redisCommand(c,cmd);
        freeReplyObject(reply);
        cout<<"insert "<<id<<":"<<value<<" to "<<pid<<endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    redisFree(c);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    cout<<"******load completed******"<<endl;

    //cout<<"*********************************************"<<endl;




    std::mutex dataMutex;
    int minSize = 10;
    redisContext *c1;
    redisContext *c2;
    redisReply *reply1;
    redisReply *reply2;

    double DELETE = 0.3;
    double CHANGE = 0.3;
    double DEL_INSERT = 0.4;
    double DEL_CHANGE = 0.3;
    double INDIRECT = 0.5;


    std::thread thread_a([&]{
        
        c1 = redisConnect("192.168.192.133", 6379);
        c2 = redisConnect("192.168.192.133", 6380);
        timeval t1{}, t2{};
        while (1)
        {
            //std::this_thread::sleep_for(std::chrono::microseconds(5000));
            //std::this_thread::sleep_for(std::chrono::milliseconds(20));
            //std::lock_guard<std::mutex> lg(dataMutex);
            dataMutex.lock();
            gettimeofday(&t1, nullptr);
            
            if(!treeMentain(t,uid,gen,c1,15)){

/*                
                double rd = decide();

                if (rd < DELETE){
                    if (rd < DELETE*DEL_INSERT){
                        if (rd < DELETE*DEL_INSERT*INDIRECT){
                            test_deleteinsertindirect_conflict(t,uid,gen,c1,c2);
                        } else {
                            test_deleteinsert_conflict(t,uid,gen,c1,c2);
                        }
                    } else if (rd < DELETE*(DEL_INSERT+DEL_CHANGE)){
                        if (rd < DELETE*(DEL_INSERT+DEL_CHANGE)*INDIRECT){
                            test_deletechangeindirect_conflict(t,uid,gen,c1,c2);
                        } else {
                            test_deletechange_conflict(t,uid,gen,c1,c2);
                        }
                    } else {
                        test_deletedelete_conflict(t,uid,gen,c1,c2);
                    }

                } else if (DELETE+CHANGE) {
                    test_changechange_conflict(t,uid,gen,c1,c2);
                } else {
                    string pid = t->getInsertParentId();
                    string id = uid->Next();
                    string value = gen->buildKeyName();
                    t->insert(pid,id,value);
                    char cmd[COMMAND_LEN];
                    sprintf(cmd,"treeinsertwithid t %s %s %s",value.c_str(),pid.c_str(),id.c_str());
                    reply = (redisReply *) redisCommand(c1,cmd);
                    if (!strcmp(reply->str,"OK")){
                        printf("insert %s %s %s\n",pid.c_str(),id.c_str(),value.c_str());
                    }
                    freeReplyObject(reply);
                }
*/
                test_deletedelete_conflict(t,uid,gen,c1,c2);
            }
           

            //random_device rd;
            //mt19937 gen(rd());
            //uniform_real_distribution<>
            //test_deleteinsertindirect_conflict(t,uid,gen,c1,c2);  
            
            dataMutex.unlock();
            gettimeofday(&t2, nullptr);
            auto slp_time = (t1.tv_sec-t2.tv_sec)*SEC_TO_MICRO + (t1.tv_usec-t2.tv_usec) +
                                (long) INTERVAL_TIME; 
            this_thread::sleep_for(chrono::microseconds(slp_time>0?slp_time:1));
        
            
        }
        
        redisFree(c1);
        redisFree(c2);

        

    });

    /*    std::thread thread_b([&]{

        while(1){
            //std::this_thread::sleep_for(std::chrono::milliseconds(20));
            //std::lock_guard<std::mutex> lg(dataMutex);
            //dataMutex.lock();
            
            //dataMutex.unlock(); 
        }  
    });
    */
    thread_a.detach();
    //thread_b.join();
    
    
    int cnt = 0;
    while(1){
        

        std::this_thread::sleep_for(std::chrono::seconds(1));
            dataMutex.lock();
            //std::lock_guard<std::mutex> lg(dataMutex);
            //char *ip = "192.168.63.132";
                
            
            
            char dir[64];
            sprintf(dir,"./result/output(%d)",cnt);
            mkdir("./result/",S_IRWXU|S_IRWXG|S_IRWXO);
            mkdir(dir,S_IRWXU|S_IRWXG|S_IRWXO);
            writeTree_local(t,dir);
            for (int i=0;i<5;i++){
                write_redisReply(dir,"192.168.192.133",6379+i);
            }
            cnt++;
            
            dataMutex.unlock();
    }  
    

    return 0;
}