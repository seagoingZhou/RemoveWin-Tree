#ifndef BENCH_SET_GENERATOR_H
#define BENCH_SET_GENERATOR_H

#include "../constants.h"
#include "../util.h"
#include "set_basic.h"
#include "set_cmd.h"
#include "set_log.h"


class set_generator : public generator<string>
{
private:
    record_for_collision record_sadd, record_srem, record_sunion, record_sinter, record_sdiff;
    set_log &ele;

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

public:
    set_generator(set_log &e) : ele(e)
    {
        add_record(record_sadd);
        add_record(record_srem);
        add_record(record_sunion);
        add_record(record_sinter);
        add_record(record_sdiff);
        start_maintaining_records();
    }

    int gen_and_exec(redisContext *c) override;

};


#endif //BENCH_RPQ_GENERATOR_H
