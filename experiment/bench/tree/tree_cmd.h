#ifndef TREE_CMD_H
#define TREE_CMD_H

#include "../util.h"
#include "tree_log.h"
#include "tree_basic.h"

class tree_cmd : public cmd
{
private:
    static mutex mtx;
    op_type t;
    string pid;
    string uid;
    string value;
    tree_log &ele;
    static timeval tStart;
    static timeval tEnd;

public:

    

    tree_cmd(op_type t,string pid,string uid,string value,tree_log &em)
     : t(t),pid(pid),uid(uid),value(value), ele(em) {
         
     }

    tree_cmd(const tree_cmd &c) = default;

    int exec(redisContext *c) override;

    static void setStartTime() {
        lock_guard<mutex> lk(mtx);
        gettimeofday(&tStart, nullptr);
    }

    static void showTotalTime() {
        lock_guard<mutex> lk(mtx);
        gettimeofday(&tEnd, nullptr);
        double time_diff_sec = (tEnd.tv_sec - tStart.tv_sec) + (tEnd.tv_usec - tStart.tv_usec) / 1000000.0;
        printf("total time: %fs\n", time_diff_sec);
    }

};


#endif 