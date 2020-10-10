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

    record_for_collision add, rem, change;
    tree_log &ele;
    Uid *uid;
    rwtree_type zt;
    

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
    tree_generator(rwtree_type zt, tree_log &e) :zt(zt), ele(e)
    {
        add_record(add);
        add_record(rem);
        add_record(change);
        uid = new Uid();
        start_maintaining_records();
    }

    void gen_and_exec(redis_client &c) override;

};


#endif //BENCH_RPQ_GENERATOR_H
