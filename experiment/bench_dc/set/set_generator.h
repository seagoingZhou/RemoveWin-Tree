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
    record_for_collision sadd, srem, sunion, sinter, sdiff;
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
        add_record(sadd);
        add_record(srem);
        add_record(sunion);
        add_record(sinter);
        add_record(sdiff);
        start_maintaining_records();
    }

    void gen_and_exec(redisContext *c) override;

};


#endif //BENCH_RPQ_GENERATOR_H
