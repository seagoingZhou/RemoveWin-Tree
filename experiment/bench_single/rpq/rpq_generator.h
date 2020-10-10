//
// Created by user on 18-11-14.
//

#ifndef BENCH_RPQ_GENERATOR_H
#define BENCH_RPQ_GENERATOR_H

#include "../constants.h"
#include "../util.h"
#include "rpq_basics.h"
#include "rpq_cmd.h"
#include "rpq_log.h"


class rpq_generator : public generator<int>
{
private:
    /*
    class e_inf
    {
        unordered_set<int> h;
        vector<int> a;
        mutex mtx;
    public:
        void add(int name)
        {
            mtx.lock();
            if (h.find(name) == h.end())
            {
                h.insert(name);
                a.push_back(name);
            }
            mtx.unlock();
        }

        int get()
        {
            int r;
            mtx.lock();
            if (a.empty())
                r = -1;
            else
                r = a[intRand(static_cast<const int>(a.size()))];
            mtx.unlock();
            return r;
        }

        void rem(int name)
        {
            mtx.lock();
            auto f = h.find(name);
            if (f != h.end())
            {
                h.erase(f);
                for (auto it = a.begin(); it != a.end(); ++it)
                    if (*it == name)
                    {
                        a.erase(it);
                        break;
                    }
            }
            mtx.unlock();
        }
    };
    */

    record_for_collision add, rem;
    rpq_log &ele;
    rpq_type zt;

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
    rpq_generator(rpq_type zt, rpq_log &e) : zt(zt), ele(e)
    {
        add_record(add);
        add_record(rem);
        start_maintaining_records();
    }

    void gen_and_exec(redis_client &c) override;

};


#endif //BENCH_RPQ_GENERATOR_H
