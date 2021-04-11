#ifndef _KEY_H_
#define _KEY_H_

#include <string>
#include "../util.h"
using namespace std;

class Key {
private:
    int keyNum;

    public:
    Key(int kn):keyNum(kn) {}
    string randomKey() {
        int randNum = intRand(0, keyNum);
        string prev("#");
        return prev + to_string(randNum);
    }
    string nextKey() {
        string prev("#");
        return prev + to_string(++keyNum);
    }
};

#endif
