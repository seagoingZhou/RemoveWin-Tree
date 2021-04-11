#ifndef SET_LOG_H
#define SET_LOG_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <algorithm>
#include <mutex>
#include "set_basic.h"
#include "../util.h"
#include "key.h"

using namespace std;

class set_log : public rdt_log
{
private:
    unordered_map<string,unordered_set<string>*> *setMap;
    int setSize;
    Key *keyGen;
    int initKeySize;
    mutex mtx,m_mtx;
    vector<vector<int>> s_log;
    int upbound;
    int downbound;
    unordered_set<string> upboundSets;
    unordered_set<string> downboundSets;
    unordered_set<string> remoteMaxSets;
    unordered_set<string> remoteMinSets;
    vector<string> ovhdRecord;

    int testModel;
    int simpleFlag;

    int targetFlag;

    void checkUpAndDown(string setName);

public:
    set_log(const char *type, const char *dir, int ssize, int ksize, int up, int down, int model) : 
        rdt_log(type, dir), 
        setSize(ssize), 
        setMap(new unordered_map<string,unordered_set<string>*>()),
        initKeySize(ksize), upbound(up), downbound(down),
        testModel(model) {
        if (testModel == SIMPLE) {
            setSize = 1;
            ksize = 1000;
            simpleFlag = 0;
        }
        targetFlag = 0;
        keyGen = new Key(10000);
        for (int i = 0; i < setSize; ++i) {
            string setName = "set" + to_string(i);
            setMap->insert({setName, new unordered_set<string>()});
        }
    }

    ~set_log(){
        delete keyGen;
        for (int i = 0; i < setSize; ++i) {
            string setName = "set" + to_string(i);
            delete setMap->at(setName);
        }
        delete setMap;
    }

    void sadd(string setName, string key);
    void srem(string setName, string key);
    void sunion(string setDst, string setSrc);
    void sinter(string setDst, string setSrc);
    void sdiff(string setDst, string setSrc);
    void smembers(string setName, redisReply *reply);
    void smembers(string setName);
    void initSet();
    void write_file();

    string randomSetGet();
    string randomSetNextGet(string set);
    vector<string> randomSetGet2();
    vector<string> getDiffAndInterSets();
    vector<string> getUnionSets();
    string randomKeyGet(string set);
    string nextKeyGenerator();

    void soverhead(redisReply *reply) {
        //if (reply->type == REDIS_REPLY_DOUBLE) {
            //double count = reply->dval;
            string cnt = reply->str;
            ovhdRecord.push_back(cnt);
        //}
    }

    int getModel() {
        return testModel;
    }

    int getSimpleKeySize() {
        lock_guard<mutex> lk(mtx);
        string setName = "set0";
        return setMap->at(setName)->size();
    }

    void setSimpleFlag(int flag) {
        lock_guard<mutex> lk(mtx);
        simpleFlag = flag;
    }

    int getSimpleFlag() {
        lock_guard<mutex> lk(mtx);
        return simpleFlag;
    }

    int getTargetSize() {
        lock_guard<mutex> lk(mtx);
        string setName = "set0";
        return setMap->at(setName)->size();
    }

    void setTargetFlag(int flag) {
        lock_guard<mutex> lk(mtx);
        targetFlag = flag;
    } 

    int getTargetFlag() {
        lock_guard<mutex> lk(mtx);
        return targetFlag;
    }

    

    string getSetType() {
        lock_guard<mutex> lk(mtx);
        return type;
    }

    int getSetNumber() {
        lock_guard<mutex> lk(mtx);
        return setSize;
    }

    int getUpSize() {
        lock_guard<mutex> lk(mtx);
        return upboundSets.size();
    }

    bool isUpboundOver() {
        lock_guard<mutex> lk(mtx);
        return upboundSets.size() >= setSize / 10;
    }

    string getRemSetName() {
        lock_guard<mutex> lk(mtx);
        int idx = intRand(setSize - 1);

        string res = "set" + to_string(idx);
        if (!upboundSets.empty()) {
            res = *upboundSets.begin();
        }
        return res;
    }

    bool isDownboundOver() {
        lock_guard<mutex> lk(mtx);
        return downboundSets.size() >= setSize / 10;
    }

    int getDownSize() {
        lock_guard<mutex> lk(mtx);
        return downboundSets.size();
    }

    string getAddSetName() {
        lock_guard<mutex> lk(mtx);
        int idx = intRand(setSize - 1);
        string res = "set" + to_string(idx);;
        if (!downboundSets.empty()) {
            res = *downboundSets.begin();
        }
        return res;
    }
    
    void remoteMaxAdd(string setName) {
        lock_guard<mutex> lk(mtx);
        remoteMaxSets.insert(setName);

    }

    void remoteMinAdd(string setName) {
        lock_guard<mutex> lk(mtx);
        remoteMinSets.insert(setName);

    }
};


#endif