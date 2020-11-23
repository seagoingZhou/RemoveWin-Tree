#include "set_generator.h"

int set_generator::gen_and_exec(redisContext *c)
{
    set_op_type t;
    string set0;
    string set1;
    string key;
    double rand = decide();

    if (rand <= PADD) {
        t = ADD;
        set0 = ele.randomSetGet();
        key = ele.nextKeyGenerator();
    } else if (rand <= PREM) {
        t = REM;
        set0 = ele.randomSetGet();
        key = ele.randomKeyGet(set0);
    } else if (rand <= PUNION) {
        t = UNION;
        vector<string> sets = ele.randomSetGet2();
        set0 = sets[0];
        set1 = sets[1];
        double conf = decide();

        if (conf < PR_UNION_INTER) {
            set1 = sinter.get(sets[1]);
        } else if (conf < PR_UNION_INTER + PR_UNION_DIFF) {
            set1 = sdiff.get(sets[1]);
        }
        sunion.add(set0);
    } else if (rand <= PINTER) {
        t = INTER;
        vector<string> sets = ele.randomSetGet2();
        set0 = sets[0];
        set1 = sets[1];
        double conf = decide();
        if (conf < PR_UNION_INTER) {
            set1 = sunion.get(sets[1]);
        } else if (conf < PR_UNION_INTER + PR_INTER_DIFF) {
            set1 = sdiff.get(sets[1]);
        }
        sinter.add(set0);
    } else {
        t = DIFF;
        vector<string> sets = ele.randomSetGet2();
        set0 = sets[0];
        set1 = sets[1];
        double conf = decide();
        if (conf < PR_UNION_DIFF) {
            set1 = sunion.get(sets[1]);
        } else if (conf < PR_UNION_DIFF + PR_INTER_DIFF) {
            set1 = sinter.get(sets[1]);
        }
        sdiff.add(set0);
    }
        
    
    
    set_cmd(ele.getSetType(), t, set0, set1, key, ele).exec(c);
    return 0;
}
