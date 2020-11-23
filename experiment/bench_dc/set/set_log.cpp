#include "set_log.h"
#include <string.h>
#include <string>
#include <iostream>

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include "../../redis-4.0.8/deps/hiredis/hiredis.h"
#include <direct.h>

#endif

using namespace std;




void set_log::sadd(string setName, string key) {
    lock_guard<mutex> lk(mtx);
    setMap[setName].insert(key);
}

void set_log::srem(string setName, string key) {
    lock_guard<mutex> lk(mtx);
    setMap[setName].erase(key);
}

void set_log::sunion(string setDst, string setSrc) {
    lock_guard<mutex> lk(mtx);
    for (auto key = setMap[setSrc].begin(); key!=setMap[setSrc].end(); key++) {
        setMap[setDst].insert(*key);
    }
}
void set_log::sinter(string setDst, string setSrc) {
    lock_guard<mutex> lk(mtx);
    unordered_set<string> tmp;
    for (auto key = setMap[setDst].begin(); key!=setMap[setDst].end(); ++key) {
        if (setMap[setSrc].count(*key)) {
            tmp.insert(*key);
        }
    }
    setMap[setDst].clear();
    for (auto key = tmp.begin(); key!=tmp.end(); ++key) {
        setMap[setDst].insert(*key);
    }

}
void set_log::sdiff(string setDst, string setSrc) {
    lock_guard<mutex> lk(mtx);
    for (auto key = setMap[setSrc].begin(); key!=setMap[setSrc].end(); key++) {
        setMap[setDst].erase(*key);
    }
}

void set_log::initSet() {
    redisReply *reply;
    redisContext *c = redisConnect("192.168.192.1", 6379);
    printf("set init begin...\n");
    for (int i = 0; i < setSize; ++i) {
        string setName = setNames[i];
        for (int j = 0; j < initKeySize; ++j) {
            char tmp[64];
            string keyName = keyGen->randomKey();
            
            sprintf(tmp, "%ssadd %s %s", type, setName.c_str(), keyName.c_str());
            reply = (redisReply *) redisCommand(c, tmp);
            if (reply == nullptr) {
                printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, tmp);
                exit(-1);
            }
            if (strcmp(reply->str,"OK") == 0) {
                sadd(setName, keyName);
            }
            freeReplyObject(reply);
            std::this_thread::sleep_for(std::chrono::microseconds(2));
        }
    }
    redisFree(c);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    printf("set init finished...\n");
}

string set_log::randomSetGet() {
    lock_guard<mutex> lk(mtx);
    int idx = intRand(setSize);
    return setNames[idx];
}

vector<string> set_log::randomSetGet2() {
    lock_guard<mutex> lk(mtx);
    int idx = intRand(setSize);
    int jdx = intRand(setSize);
    while (jdx == idx) {
        jdx = intRand(setSize);
    }
    return {setNames[idx], setNames[jdx]};
}

string set_log::randomKeyGet(string set) {
    lock_guard<mutex> lk(mtx);
    int n = setMap[set].size();
    if (n == 0) {
        return "##";
    }
    int randIdx = intRand(setMap[set].size());
    unordered_set<string>::iterator it = setMap[set].begin();
    while (randIdx-- > 0 && it != setMap[set].end()) {
        ++it;
    }
    string res = *it;
    
    return res;
}

string set_log::nextKeyGenerator() {
    lock_guard<mutex> lk(mtx);
    return keyGen->nextKey();
}

void set_log::smembers(string setName, redisReply *reply) {
    vector<int> record(3);
    {
        lock_guard<mutex> lk(mtx);
        record[0] = setMap[setName].size();

        int readSum = 0;
        int commonSum = 0;

        if (reply->type == REDIS_REPLY_ARRAY) {
            readSum = reply->elements;
            for (int i = 0; i < reply->elements; i++) {
                char tmpChar[64];
                strcpy(tmpChar, reply->element[i]->str);
                string key = string(tmpChar);
                if (setMap[setName].count(key) > 0) {
                    ++commonSum;
                }
            }
        }
        record[1] = readSum;
        record[2] = commonSum;
    }

    {
        lock_guard<mutex> lk(m_mtx);
        s_log.push_back(record);
    }
}


void set_log::write_file() {
    char n[64], f[64];
    sprintf(n, "%s/%s-%d-%d-%d-%d", dir, type, TOTAL_SERVERS, OP_PER_SEC, DELAY, DELAY_LOW);
    bench_mkdir(n);

    sprintf(f, "%s/s.set", n);
    FILE *setLog = fopen(f, "w");
    for (vector<int> record : s_log) {
        fprintf(setLog, "%d %d %d\n", record[0], record[1], record[2]);
    }
    fflush(setLog);
    fclose(setLog);
    
}
