#include "set_generator.h"

int set_generator::gen_and_exec(redisContext *c)
{
    set_op_type t;
    string set0;
    string set1;
    string key;
    double rand = decide();
    if (ele.getModel() == SIMPLE) {
        set0 = "set0";
        int keySize = ele.getSimpleKeySize();
        if (keySize < SIMPLE_MIN || ele.getSimpleFlag() == -1) {
            t = ADD;
            key = ele.nextKeyGenerator();
            record_sadd.add(key);
        } else if (keySize > SIMPLE_MAX || ele.getSimpleFlag() == 1) {
            t = REM;
            key = ele.randomKeyGet(set0);
            record_srem.add(key);
        } else if (rand <= PADD) {
            t = ADD;
            double conf = decide();
            key = ele.nextKeyGenerator();
            if (conf <= P_ADD_REM) {
                key = record_srem.get(key);
            }
            record_sadd.add(key);
        } else {
            t = REM;
            double conf = decide();
            key = ele.randomKeyGet(set0);
            if (conf <= P_ADD_REM) {
                key = record_sadd.get(key);
            }
            record_srem.add(key);

        }
    } else {
        set0 = "set0";
        int targetSize = ele.getTargetSize();
        int targetFlag = ele.getTargetFlag();
        if (targetSize > MAX_KEY_SIZE || targetFlag == 1) {
            double conf = decide();
            if (conf <= 0.5) {
                t = REM;
                key = ele.randomKeyGet(set0);
            } else {
                t = DIFF;
                set1 = ele.randomSetNextGet(set0);
            }
            
        } else if (targetSize < MIN_KEY_SIZE || targetFlag == -1) {
            double conf = decide();
            if (conf <= 0.5) {
                t = ADD;
                key = ele.nextKeyGenerator();
            } else {
                t = UNION;
                set1 = ele.randomSetNextGet(set0);
            }
        } else {
            if (rand <= PADD) {
                t = ADD;
                set0 = ele.getAddSetName();
                key = ele.nextKeyGenerator();
            } else if (rand <= PREM) {
                t = REM;
                set0 = ele.getRemSetName();
                key = ele.randomKeyGet(set0);
            } else if (rand <= PUNION) {
                t = UNION;
                set1 = ele.randomSetNextGet(set0);
            } else if (rand <= PINTER) {
                t = INTER;
                set1 = ele.randomSetNextGet(set0);
            } else {
                t = DIFF;
                set1 = ele.randomSetNextGet(set0);
            }
        }
    }

    set_cmd(ele.getSetType(), t, set0, set1, key, ele).exec(c);
    return 0;
}

/*
if (ele.isUpboundOver() || rand <= PREM) {
            t = REM;
            set0 = ele.getRemSetName();
            key = ele.randomKeyGet(set0);
        } else if (ele.isDownboundOver() || rand <= PADD) {
            t = ADD;
            set0 = ele.getAddSetName();
            key = ele.nextKeyGenerator();
        }  else if (rand <= PUNION) {
            t = UNION;
            vector<string> sets;
            sets = ele.getUnionSets();
            if (sets.empty()) {
                sets = ele.randomSetGet2();
            } else if (sets.size() == 1) {
                string tmpKey = ele.randomSetNextGet(sets[0]);
                sets.push_back(tmpKey);
            }
            set0 = sets[0];
            set1 = sets[1];
            
            double conf = decide();
            if (conf < PR_UNION_INTER) {
                set1 = sinter.get(sets[1]);
            } else if (conf < PR_UNION_INTER + PR_UNION_DIFF) {
                set1 = sdiff.get(sets[1]);
            }
            
            record_sunion.add(set0);
        } else if (rand <= PINTER) {
            t = INTER;
            vector<string> sets;
            sets = ele.getDiffAndInterSets();
            if (sets.empty()) {
                sets = ele.randomSetGet2();
            } else if (sets.size() == 1) {
                string tmpKey = ele.randomSetNextGet(sets[0]);
                sets.push_back(tmpKey);
            }
            set0 = sets[0];
            set1 = sets[1];
            
            double conf = decide();
            if (conf < PR_UNION_INTER) {
                set1 = sunion.get(sets[1]);
            } else if (conf < PR_UNION_INTER + PR_INTER_DIFF) {
                set1 = sdiff.get(sets[1]);
            }
            
            record_sinter.add(set0);
        } else {
            t = DIFF;
            vector<string> sets;
            sets = ele.getDiffAndInterSets();
            if (sets.empty()) {
                sets = ele.randomSetGet2();
            } else if (sets.size() == 1) {
                string tmpKey = ele.randomSetNextGet(sets[0]);
                sets.push_back(tmpKey);
            }
            set0 = sets[0];
            set1 = sets[1];
            
            double conf = decide();
            if (conf < PR_UNION_DIFF) {
                set1 = sunion.get(sets[1]);
            } else if (conf < PR_UNION_DIFF + PR_INTER_DIFF) {
                set1 = sinter.get(sets[1]);
            }
            
            record_sdiff.add(set0);
            */
