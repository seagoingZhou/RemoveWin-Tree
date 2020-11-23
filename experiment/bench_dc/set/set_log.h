#ifndef SET_LOG_H
#define SET_LOG_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
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
    vector<string> setNames;
    unordered_map<string,unordered_set<string>> setMap;
    int setSize;
    Key *keyGen;
    int initKeySize;
    mutex mtx,m_mtx;
    vector<vector<int>> s_log;

public:
    set_log(const char *type, const char *dir, int ssize, int ksize) : 
        rdt_log(type, dir), setSize(ssize), setNames(ssize), initKeySize(ksize) {
        
        keyGen = new Key(1000);
        for (int i = 0; i < setSize; ++i) {
            string setName = "set" + to_string(i);
            setNames[i] = setName;
            unordered_set<string> tmp;
            setMap[setName] = tmp;
        }
    }

    ~set_log(){
        delete keyGen;
    }

    void sadd(string setName, string key);
    void srem(string setName, string key);
    void sunion(string setDst, string setSrc);
    void sinter(string setDst, string setSrc);
    void sdiff(string setDst, string setSrc);
    void smembers(string setName, redisReply *reply);
    void initSet();
    void write_file();

    string randomSetGet();
    vector<string> randomSetGet2();
    string randomKeyGet(string set);
    string nextKeyGenerator();

    string getSetType() {
        return type;
    }
};


#endif