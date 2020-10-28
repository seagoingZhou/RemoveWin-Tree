#ifndef BENCH_TREE_GENERATOR_H
#define BENCH_TREE_GENERATOR_H

#include "../constants.h"
#include "../util.h"
#include "tree_basic.h"
#include "tree_cmd.h"
#include "tree_log.h"
#include "uid.h"


class tree_generator : public generator<string>
{
private:
    record_for_collision add, rem, change;
    tree_log &ele;
    Uid *uid;

    static int gen_element()
    {
        return intRand(MAX_ELE);
    }

    static double gen_initial()
    {
        return doubleRand(0, MAX_INIT);
    }

    static double gen_increament()
    {
        return doubleRand(-MAX_INCR, MAX_INCR);
    }

    static string gen_value(){
        int keynum = intRand(MAX_ELE);
        int zeropading = 8;
        string value = to_string(keynum);
        int fill = zeropading - value.size();

        string prev = "node";
        string pading(fill,'0');

        return prev+pading+value;
    }

    string gen_uid(){
        return uid->Next();
    }

public:
    tree_generator(tree_log &e) : ele(e)
    {
        add_record(add);
        add_record(rem);
        add_record(change);
        uid = new Uid();
        start_maintaining_records();
    }

    int gen_and_exec(redisContext *c) override;

};


#endif //BENCH_RPQ_GENERATOR_H
