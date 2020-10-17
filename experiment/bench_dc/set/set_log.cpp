#include "set_log.h"
#include <string.h>
#include <string>
#include <iostream>

using namespace std;


string set_log::randomSetGet() {
    lock_guard<mutex> lk(mtx);
    int idx = intRand(setSize);
    return setNames[idx];
}

vector<string> set_log::randomSetGet2() {
    lock_guard<mutex> lk(mtx);
    int idx = intRand(setSize);
    int jdx = intRand(setSize);
    while (jdx = idx) {
        jdx = intRand(setSize);
    }
    return {setNames[idx], setNames[jdx]};
}

string set_log::randomKeyGet(string set) {
    lock_guard<mutex> lk(mtx);
    int randIdx = intRand(setMap[set].size());
    unordered_set<string>::iterator it = setMap[set].begin();
    while (randIdx-- > 0) {
        ++it;
    }
    string res = *it;
    return res;
}

string set_log::randomKeyGenerator(string set) {
    int idx = intRand(maxKeySize);
    string res = "#" + set + "@" + to_string(idx);
    return res;
}


void set_log::writeFile(){
    
}
