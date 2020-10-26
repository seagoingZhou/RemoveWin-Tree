#ifndef SET_CMD_H
#define SET_CMD_H

#include "../util.h"
#include "set_log.h"

class set_cmd : public cmd
{
private:
    set_op_type t;
    string set0;
    string set1;
    string key;
    set_log &ele;

public:

    set_cmd(set_op_type it,string iset0,string iset1,string ikey,set_log &iele)
     : t(t),set0(iset0),set1(iset1),key(ikey), ele(iele) {}

    set_cmd(const set_cmd &c) = default;

    void exec(redisContext *c) override;
};


#endif 