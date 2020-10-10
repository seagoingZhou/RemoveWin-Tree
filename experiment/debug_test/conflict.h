#include "generator/generator.h"
#include "generator/uniform_generator.h"
#include "generator/zipfian_generator.h"
#include "generator/core_generator.h"
#include "uid.h"
#include "tree.h"

#include <hiredis/hiredis.h>
#include <string.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>

using namespace ycsbc;
using namespace std;

#define COMMAND_LEN 128

int ConcurrentChangeAndChange(redisContext *c1,redisContext *c2,
                string id,string val1, string val2){
    
    redisReply *reply1;
    redisReply *reply2;
    int ret = 0;

    char cmd1[COMMAND_LEN];
    char cmd2[COMMAND_LEN];
    sprintf(cmd1,"treechangevalue t %s %s",id.c_str(),val1.c_str());
    sprintf(cmd2,"treechangevalue t %s %s",id.c_str(),val2.c_str());
    reply1 = (redisReply *) redisCommand(c1,cmd1);  
    reply2 = (redisReply *) redisCommand(c2,cmd2);

    int flag_cmd1 = !strcmp(reply1->str,"OK");
    int flag_cmd2 = !strcmp(reply2->str,"OK");

    if (flag_cmd1&&flag_cmd2){
        ret = 3;
    } else if (flag_cmd1){
        ret = 2;
    } else if (flag_cmd2){
        ret = 1;
    } 

    freeReplyObject(reply1);
    freeReplyObject(reply2);

    return ret;

}

void test_changechange_conflict(Tree *t, Uid *uid,CoreGenerator *gen,
                 redisContext *c1,redisContext *c2){

    string id = t->getParentId();
    string val1 = gen->buildKeyName();
    string val2 = gen->buildKeyName();

    int flag = ConcurrentChangeAndChange(c1,c2,id,val1,val2);
    if (flag == 3){
        string name = val1<val2?val1:val2;
        t->changevalue(id,name);
        printf("change %s %s || change %s %s\n",
                id.c_str(),val1.c_str(),id.c_str(),val2.c_str());
    } else if(flag == 2){
        t->changevalue(id,val1);
        printf("change %s %s\n",
                id.c_str(),val1.c_str());
    } else if (flag == 1){
        t->changevalue(id,val2);
        printf("change %s %s\n",
                id.c_str(),val2.c_str());
    }

        

}



int ConcurrentDeleteAndChange(redisContext *c1,redisContext *c2,
                string id,string val){

    redisReply *reply1;
    redisReply *reply2;
    int ret = 0;

    char cmd1[COMMAND_LEN];
    char cmd2[COMMAND_LEN];
    sprintf(cmd1,"treedelete t %s",id.c_str());
    sprintf(cmd2,"treechangevalue t %s %s",id.c_str(),val.c_str());
    reply1 = (redisReply *) redisCommand(c1,cmd1);
    reply2 = (redisReply *) redisCommand(c2,cmd2);

    int flag_cmd1 = !strcmp(reply1->str,"OK");
    int flag_cmd2 = !strcmp(reply2->str,"OK");

    if (flag_cmd1&&flag_cmd2){
        ret = 3;
    } else if (flag_cmd1){
        ret = 2;
    } else if (flag_cmd2){
        ret = 1;
    } 

    freeReplyObject(reply1);
    freeReplyObject(reply2);

    return ret;

}

void test_deletechange_conflict(Tree *t, Uid *uid,CoreGenerator *gen,
                 redisContext *c1,redisContext *c2){
  
    string id = t->getParentId();
    string val = gen->buildKeyName();   

    int flag = ConcurrentDeleteAndChange(c1,c2,id,val);
    if (flag == 3){
        t->del(id);
        printf("delete %s || change %s %s\n",
                id.c_str(),id.c_str(),val.c_str());
    } else if (flag == 2){
        t->del(id);
        printf("delete %s\n",
                id.c_str());
    }else if (flag == 1){
        t->changevalue(id,val);
        printf("change %s %s\n",
                id.c_str(),val.c_str());
    }       
    
    

}

int ConcurrentDeleteAndChangeIndirect(redisContext *c1,redisContext *c2,
                string id0,string id1,string val){

    redisReply *reply1;
    redisReply *reply2;
    int ret = 0;

    char cmd1[COMMAND_LEN];
    char cmd2[COMMAND_LEN];
    sprintf(cmd1,"treedelete t %s",id0.c_str());
    sprintf(cmd2,"treechangevalue t %s %s",id1.c_str(),val.c_str());
    reply1 = (redisReply *) redisCommand(c1,cmd1); 
    reply2 = (redisReply *) redisCommand(c2,cmd2);

    int flag_cmd1 = !strcmp(reply1->str,"OK");
    int flag_cmd2 = !strcmp(reply2->str,"OK");

    if (flag_cmd1&&flag_cmd2){
        ret = 3;
    } else if (flag_cmd1){
        ret = 2;
    } else if (flag_cmd2){
        ret = 1;
    } 

    freeReplyObject(reply1);
    freeReplyObject(reply2);

    return ret;

}

void test_deletechangeindirect_conflict(Tree *t, Uid *uid,CoreGenerator *gen,
                 redisContext *c1,redisContext *c2){

    
    string id = t->getDeleteId();
        
    vector<string> st = t->subtree(id);
    string de_id = st[st.size()-1]; 
    string val = gen->buildKeyName(); 

    int flag = ConcurrentDeleteAndChangeIndirect(c1,c2,id,de_id,val); 
    if (flag == 3){
        t->del(id);
        printf("delete %s || change %s %s\n",
                id.c_str(),de_id.c_str(),val.c_str());
    } else if (flag == 2){
        t->del(id);
        printf("delete %s\n",
                id.c_str());
    }else if (flag == 1){
        t->changevalue(de_id,val);
        printf("change %s %s\n",
                de_id.c_str(),val.c_str());
    }        
        
}

int ConcurrentDeleteAndInsert(redisContext *c1,redisContext *c2,
                string id0,string id1,string val){
    redisReply *reply1;
    redisReply *reply2;
    int ret = 0;

    char cmd1[COMMAND_LEN];
    char cmd2[COMMAND_LEN];
    sprintf(cmd1,"treedelete t %s",id0.c_str());
    sprintf(cmd2,"treeinsertwithid t %s %s %s",val.c_str(),id0.c_str(),id1.c_str());
    reply1 = (redisReply *) redisCommand(c1,cmd1);
    reply2 = (redisReply *) redisCommand(c2,cmd2);

    int flag_cmd1 = !strcmp(reply1->str,"OK");
    int flag_cmd2 = !strcmp(reply2->str,"OK");

    if (flag_cmd1&&flag_cmd2){
        ret = 3;
    } else if (flag_cmd1){
        ret = 2;
    } else if (flag_cmd2){
        ret = 1;
    } 

    freeReplyObject(reply1);
    freeReplyObject(reply2);
    return ret;
}

void test_deleteinsert_conflict(Tree *t, Uid *uid,CoreGenerator *gen,
                 redisContext *c1,redisContext *c2){

    
    string pid = t->getParentId();

    string id = uid->Next();
    string val = gen->buildKeyName();   

    int flag = ConcurrentDeleteAndInsert(c1,c2,pid,id,val);
    if (flag == 3){
        t->del(pid);
        printf("delete %s || insert %s %s %s\n",
                pid.c_str(),pid.c_str(),id.c_str(),val.c_str());
    } else if (flag == 2){
        t->del(pid);
        printf("delete %s\n",
                pid.c_str());
    }else if (flag == 1){
        t->insert(pid.c_str(),id.c_str(),val.c_str());
        printf("insert %s %s %s\n",
                pid.c_str(),id.c_str(),val.c_str());
    }        

    //std::this_thread::sleep_for(std::chrono::milliseconds(100));

    
}

int ConcurrentDeleteAndInsertIndirect(redisContext *c1,redisContext *c2,
                string id0,string id1,string uid,string val){
    redisReply *reply1;
    redisReply *reply2;
    int ret = 0;

    char cmd1[COMMAND_LEN];
    char cmd2[COMMAND_LEN];
    sprintf(cmd1,"treedelete t %s",id0.c_str());
    sprintf(cmd2,"treeinsertwithid t %s %s %s",val.c_str(),id1.c_str(),uid.c_str());
    reply1 = (redisReply *) redisCommand(c1,cmd1);
    reply2 = (redisReply *) redisCommand(c2,cmd2);

    int flag_cmd1 = !strcmp(reply1->str,"OK");
    int flag_cmd2 = !strcmp(reply2->str,"OK");

    if (flag_cmd1&&flag_cmd2){
        ret = 3;
    } else if (flag_cmd1){
        ret = 2;
    } else if (flag_cmd2){
        ret = 1;
    } 

    freeReplyObject(reply1);
    freeReplyObject(reply2);

    return ret;

}

void test_deleteinsertindirect_conflict(Tree *t, Uid *uid,CoreGenerator *gen,
                 redisContext *c1,redisContext *c2){

    
    string id0 = t->getDeleteId();

    vector<string> st = t->subtree(id0);
    string pid = st[st.size()-1];

    string id = uid->Next();
    string val = gen->buildKeyName();   

    int flag = ConcurrentDeleteAndInsertIndirect(c1,c2,id0,pid,id,val);

    if (flag == 3){
        t->del(id0);
        printf("delete %s || insert %s %s %s\n",
                id0.c_str(),pid.c_str(),id.c_str(),val.c_str());
    } else if (flag == 2){
        t->del(id0);
        printf("delete %s\n",
                id0.c_str());
    }else if (flag == 1){
        t->insert(pid.c_str(),id.c_str(),val.c_str());
        printf("insert %s %s %s\n",
                pid.c_str(),id.c_str(),val.c_str());
    }        
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));

    
}

int ConcurrentDeleteAndDelete(redisContext *c1,redisContext *c2,
                string id0,string id1){

    redisReply *reply1;
    redisReply *reply2;
    int ret = 0;

    char cmd1[COMMAND_LEN];
    char cmd2[COMMAND_LEN];
    sprintf(cmd1,"treedelete t %s",id0.c_str());
    sprintf(cmd2,"treedelete t %s",id1.c_str());
    reply1 = (redisReply *) redisCommand(c1,cmd1); 
    reply2 = (redisReply *) redisCommand(c2,cmd2);

    int flag_cmd1 = !strcmp(reply1->str,"OK");
    int flag_cmd2 = !strcmp(reply2->str,"OK");

    if (flag_cmd1&&flag_cmd2){
        ret = 3;
    } else if (flag_cmd1){
        ret = 2;
    } else if (flag_cmd2){
        ret = 1;
    } 

    freeReplyObject(reply1);
    freeReplyObject(reply2);
    return ret;

}

void test_deletedelete_conflict(Tree *t, Uid *uid,CoreGenerator *gen,
                 redisContext *c1,redisContext *c2){

    
    string id0 = t->getDeleteId();

    vector<string> st = t->subtree(id0);
    string id1 = st.back();
    
    
    int flag = ConcurrentDeleteAndDelete(c1,c2,id0,id0);
    if (flag == 3){
        t->del(id0);
        printf("delete %s || delete %s\n",
                id0.c_str(),id0.c_str());
    } else if (flag == 2){
        t->del(id0);
        printf("delete %s\n",
                id0.c_str());
    }else if (flag == 1){
        t->del(id0);
        printf("delete %s\n",id0.c_str());
    }


    //std::this_thread::sleep_for(std::chrono::milliseconds(100));

}

int ConcurrentDeleteMove(redisContext *c1,redisContext *c2,Tree *t,
                string id0,string id1,string id2){

    int ret = 0;
    redisReply *reply1;
    redisReply *reply2;

    char cmd1[COMMAND_LEN];
    char cmd2[COMMAND_LEN];
    sprintf(cmd1,"treedelete t %s",id0.c_str());
    sprintf(cmd2,"treemove t %s %s",id1.c_str(),id2.c_str());
    reply1 = (redisReply *) redisCommand(c1,cmd1);    
    reply2 = (redisReply *) redisCommand(c2,cmd2);

    int flag_del = !strcmp(reply1->str,"OK");
    int flag_move = !strcmp(reply2->str,"OK");

    if (flag_del&&flag_move){
        ret = 3;
    } else if (flag_del){
        ret = 2;
    } else if (flag_move){
        ret = 1;
    } else {
        ret = 0;
    }

    freeReplyObject(reply1); 
    freeReplyObject(reply2);

    return ret;

}

void test_deletemove_conflict(Tree *t, Uid *uid,CoreGenerator *gen,
                 redisContext *c1,redisContext *c2,int type){

    string id0;
    string id1;
    string id2;

    string parent;
    
    do{
        id0 = t->getDeleteId();
        parent = t->tree[id0]->parent;
    }while(id0=="0,0" || parent == "0,0" 
            || t->tree[parent]->children.size()<=1);

    vector<string> subt = t->subtree(id0);

    vector<string> imFam = t->ancestors(id0);
    for (auto &it:subt){
        imFam.push_back(it);
    }
    id1 = subt.back();
    vector<string>::iterator iter;
    vector<string>::iterator iter2;
    do{
        id2 = t->getDeleteId();
        iter = find(imFam.begin(),imFam.end(),id2);
        
    }while(iter!=imFam.end());

    if (type == 0){
        //move out
        int flag = ConcurrentDeleteMove(c1,c2,t,id0,id2,id1);
        if (flag == 3 || flag == 2){
            t->del(id0);
            cout<<"delete "<<id0<<endl;
        } else if (flag == 1){
            t->move(id2,id1);
            cout<<"move "<<id1<<" to "<<id2<<endl;
        }
        
    } else {
        //move in
        int flag = ConcurrentDeleteMove(c1,c2,t,id0,id1,id2);
        if (flag == 3){
            t->del(id0);
            t->del(id2);
            cout<<"delete "<<id0<<"/"<<id1<<" and "<<id2<<endl;
        } else if(flag == 2) {
            t->del(id0);
            cout<<"delete "<<id0<<endl;
        } else if(flag == 1){
            t->move(id1,id2);
            cout<<"move "<<id2<<" to "<<id1<<endl;
        }
        
    }
}

int treeMentain(Tree *t, Uid *uid,CoreGenerator *gen,
                 redisContext *c, int minSize){

    int currentSize = t->node.size();
    redisReply *reply;
    if (currentSize < minSize){
        for (int i=0; i<50-currentSize; i++){
            string pid = t->getInsertParentId();
            string id = uid->Next();
            string value = gen->buildKeyName();
            t->insert(pid,id,value);
            char cmd[COMMAND_LEN];
            sprintf(cmd,"treeinsertwithid t %s %s %s",value.c_str(),pid.c_str(),id.c_str());
            reply = (redisReply *) redisCommand(c,cmd);
            if (!strcmp(reply->str,"OK")){
                printf("insert %s %s %s\n",pid.c_str(),id.c_str(),value.c_str());
            }
            freeReplyObject(reply);
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return 1;
    }
    return 0;
}
