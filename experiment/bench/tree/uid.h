#ifndef _UID_H_
#define _UID_H_

#include <string>
using namespace std;

class Uid{
    private:
    long long opnum;

    public:
    Uid(){
        opnum = 0;
    }
    ~Uid(){}

    string Next(){
        string prev("#");
        string num = to_string(opnum++);
        return prev+num;
    }

};

#endif

