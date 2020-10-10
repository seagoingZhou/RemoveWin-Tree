#ifndef TREE_CMD_H
#define TREE_CMD_H

#include "../util.h"
#include "tree_log.h"
#include "tree_basic.h"

class tree_cmd : public cmd
{
private:
    op_type t;
    string pid;
    string uid;
    string value;
    tree_log &ele;

public:

    tree_cmd(op_type t,string pid,string uid,string value,tree_log &em)
     : t(t),pid(pid),uid(uid),value(value), ele(em) {}

    tree_cmd(const tree_cmd &c) = default;

    void exec(redisContext *c) override;
};


#endif 