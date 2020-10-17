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

using namespace std;

class set_log : public rdt_log
{
private:
    vector<string> setNames;
    unordered_map<string,unordered_set<string>> setMap;
    int setSize;
    int maxKeySize;
    mutex mtx,m_mtx;

public:
    set_log(const char *type, const char *dir, int size) : rdt_log(type, dir), setSize(size), setNames(size) {
        for (int i = 0; i < size; ++i) {
            string setName = "set" + to_string(i);
            setNames[i] = setName;
            unordered_set<string> tmp;
            setMap[setName] = tmp;
            maxKeySize = MAX_KEY_SIZE;
        }
    }

    ~set_log(){
        
    }

    void sadd(string setName, string key);
    void srem(string setName, string key);
    void sunion(string setDst, string setSrc);
    void sinter(string setDst, string setSrc);
    void sdiff(string setDst, string setSrc);
    void smembers(string setName, redisReply *reply);
    void writeFile();

    string randomSetGet();
    vector<string> randomSetGet2();
    string randomKeyGet(string set);
    string randomKeyGenerator(string set);
};


#endif