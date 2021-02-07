#include "set_log.h"
#include <iostream>

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include "../../redis-4.0.8/deps/hiredis/hiredis.h"
#include <direct.h>

#endif

using namespace std;


void set_log::checkUpAndDown(string setName) {
    upboundSets.erase(setName);
    downboundSets.erase(setName);
    if (setMap->at(setName)->size() > upbound) {
        upboundSets.insert(setName);
    } else if (setMap->at(setName)->size() < downbound) {
        downboundSets.insert(setName);
    }
}

void set_log::sadd(string setName, string key) {
    lock_guard<mutex> lk(mtx);
    if (setMap->find(setName) == setMap->end()) {
        setMap->insert({setName, new unordered_set<string>()});
    }
    setMap->at(setName)->insert(key);
    checkUpAndDown(setName);
}

void set_log::srem(string setName, string key) {
    lock_guard<mutex> lk(mtx);
    if (setMap->find(setName) == setMap->end()) {
        setMap->insert({setName, new unordered_set<string>()});
    }
    setMap->at(setName)->erase(key);
    checkUpAndDown(setName);
}

void set_log::sunion(string setDst, string setSrc) {
    lock_guard<mutex> lk(mtx);
    if (setMap->find(setDst) == setMap->end()) {
        setMap->insert({setDst, new unordered_set<string>()});
    }
    if (setMap->find(setSrc) == setMap->end()) {
        setMap->insert({setSrc, new unordered_set<string>()});
    }
    for (auto key = setMap->at(setSrc)->begin(); key != setMap->at(setSrc)->end(); ++key) {
        setMap->at(setDst)->insert(*key);
        
    }
    checkUpAndDown(setDst);
}
void set_log::sinter(string setDst, string setSrc) {
    lock_guard<mutex> lk(mtx);
    if (setMap->find(setDst) == setMap->end()) {
        setMap->insert({setDst, new unordered_set<string>()});
    }
    if (setMap->find(setSrc) == setMap->end()) {
        setMap->insert({setSrc, new unordered_set<string>()});
    }
    unordered_set<string> *tmp = new unordered_set<string>();
    for (auto key = setMap->at(setDst)->begin(); key != setMap->at(setDst)->end(); ++key) {
        if (setMap->at(setSrc)->find(*key) != setMap->at(setSrc)->end()) {
            tmp->insert(*key);
        }
        
    }
    delete setMap->at(setDst);
    setMap->at(setDst) = tmp;
    checkUpAndDown(setDst);

}
void set_log::sdiff(string setDst, string setSrc) {
    lock_guard<mutex> lk(mtx);
    if (setMap->find(setDst) == setMap->end()) {
        setMap->insert({setDst, new unordered_set<string>()});
    }
    if (setMap->find(setSrc) == setMap->end()) {
        setMap->insert({setSrc, new unordered_set<string>()});
    }
    for (auto key = setMap->at(setSrc)->begin(); key != setMap->at(setSrc)->end(); ++key) {
        setMap->at(setDst)->erase(*key);
    }
    checkUpAndDown(setDst);
}

void set_log::initSet() {
    redisReply *reply;
    redisContext *c = redisConnect("192.168.192.1", 6379);
    printf("set init begin...\n");
    for (int j = 0; j < setSize; ++j) {
        string setName = "set" + to_string(j);
        char tmp[512];
        string keys = "";
        unordered_set<string> kSet;
        for (int k = 0; k < 25; ++k) {
            string keyName = keyGen->randomKey();
            if (!kSet.count(keyName)) {
                kSet.insert(keyName);
                sadd(setName, keyName);
                keys += " " + keyName;
            }
        }
        sprintf(tmp, "%ssadd %s %s", type, setName.c_str(), keys.c_str());
        reply = (redisReply *) redisCommand(c, tmp);
        if (reply == nullptr) {
            printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, tmp);
            exit(-1);
        }
        freeReplyObject(reply);
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    for (int i = 1; i < initKeySize / 25; ++i) {
        for (int j = 0; j < setSize; ++j) {
            string setName = "set" + to_string(j);
            char tmp[512];
            string keys = "";
            unordered_set<string> kSet;
            for (int k = 0; k < 25; ++k) {
                string keyName = keyGen->randomKey();
                if (!kSet.count(keyName)) {
                    kSet.insert(keyName);
                    sadd(setName, keyName);
                    keys += " " + keyName;
                }
            }
            sprintf(tmp, "%ssadd %s %s", type, setName.c_str(), keys.c_str());
            reply = (redisReply *) redisCommand(c, tmp);
            if (reply == nullptr) {
                printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, tmp);
                exit(-1);
            }
            freeReplyObject(reply);
            //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    redisFree(c);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    printf("set init finished...\n");
}

string set_log::randomSetGet() {
    lock_guard<mutex> lk(mtx);
    int idx = intRand(setSize - 1);
    string res = "set" + to_string(idx);;
    for (int i = 0; i < setSize; ++i) {
        int jdx = (idx + i) % setSize;
        string curSetName = "set" + to_string(jdx);
        if (!upboundSets.count(curSetName) && 
            !downboundSets.count(curSetName) &&
            !remoteMaxSets.count(curSetName) &&
            !remoteMinSets.count(curSetName)) {
            res = curSetName;
            break;
        }
    }
    return res;
}

string set_log::randomSetNextGet(string set) {
    lock_guard<mutex> lk(mtx);
    int idx = intRand(setSize - 1);
    string res = "set" + to_string(idx);;
    for (int i = 0; i < setSize; ++i) {
        int jdx = (idx + i) % setSize;
        string curSetName = "set" + to_string(jdx);
        if (curSetName != set) {
            res = curSetName;
        }
        if (!upboundSets.count(curSetName) && 
            !downboundSets.count(curSetName) &&
            !remoteMaxSets.count(curSetName) &&
            !remoteMinSets.count(curSetName) &&
            set != curSetName) {
            
            res = curSetName;
            break;
        }
    }
    return res;
}

vector<string> set_log::getDiffAndInterSets() {
    lock_guard<mutex> lk(mtx);
    vector<string> res;
    if (remoteMaxSets.empty()) {
        return res;
    } else if (remoteMaxSets.size() == 1) {
        res.push_back(*remoteMaxSets.begin());
    } else {
        auto it = remoteMaxSets.begin();
        res.push_back(*it);
        ++it;
        res.push_back(*it);
        
    }
    remoteMaxSets.erase(res[0]);
    return res;
}

vector<string> set_log::getUnionSets() {
    lock_guard<mutex> lk(mtx);
    vector<string> res;
    if (remoteMinSets.empty()) {
        return res;
    } else if (remoteMinSets.size() == 1) {
        res.push_back(*remoteMinSets.begin());
    } else {
        auto it = remoteMinSets.begin();
        res.push_back(*it);
        ++it;
        res.push_back(*it);
        
    }
    remoteMinSets.erase(res[0]);
    return res;
}

vector<string> set_log::randomSetGet2() {
    lock_guard<mutex> lk(mtx);
    int idx = intRand(setSize - 1);
    int jdx = intRand(setSize - 1);
    while (jdx == idx) {
        jdx = intRand(setSize - 1);
    }
    string res0 = "set" + to_string(idx);
    string res1 = "set" + to_string(jdx);

    return {res0, res1};
}

string set_log::randomKeyGet(string set) {
    lock_guard<mutex> lk(mtx);
    if (setMap->at(set)->empty()) {
        return "##";
    }
    /*
    int randIdx = intRand(setMap[set].size());
    unordered_set<string>::iterator it = setMap[set].begin();
    while (randIdx-- > 0 && it != setMap[set].end()) {
        ++it;
    }
    */
    string res = *setMap->at(set)->begin();
    /*
    if (it != setMap[set].end()) {
        res = *it;
    }
    */
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
        record[0] = setMap->at(setName)->size();

        int readSum = 0;
        int commonSum = 0;

        int sum = reply->elements;

        if (reply->type == REDIS_REPLY_ARRAY) {
            readSum = sum;
            /*
            if (sum > REMOTE_MAX_KEY_SIZE) {
                remoteMaxSets.insert(setName);
            } else if (sum < REMOTE_MIN_KEY_SIZE) {
                remoteMinSets.insert(setName);
            }
            */
            for (int i = 0; i < sum; i++) {
                char tmpChar[64];
                strcpy(tmpChar, reply->element[i]->str);
                string key = string(tmpChar);
                if (setMap->at(setName)->count(key) > 0) {
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
    sprintf(n, "%s/%s-%d-%d-%d-%d-%d", dir, type, TOTAL_SERVERS, OP_PER_SEC, DELAY, DELAY_LOW, ROUND);
    bench_mkdir(n);

    sprintf(f, "%s/s.set", n);
    FILE *setLog = fopen(f, "w");
    for (vector<int> record : s_log) {
        fprintf(setLog, "%d %d %d\n", record[0], record[1], record[2]);
    }
    fflush(setLog);
    fclose(setLog);
    
}
