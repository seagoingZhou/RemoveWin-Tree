#ifndef _UTIL_H
#define _UTIL_H

#include <thread>
#include <random>
#include <string>
#include <mutex>
#include <hiredis/hiredis.h>
#include <unordered_set>

#define SPLIT_NUM 10

int intRand(int min, int max);

int intRand(int max);

double doubleRand(double min, double max);

double decide();

enum tree_op_type
{
    treeinsert = 0, treedelete = 1, treechangevalue = 2, treemove = 3, treemembers = 4
};

class tree_log{

};

class cmd
{
public:
public:
    virtual bool is_null() { return false; }
    virtual void exec(redisContext *c) = 0;
};

class collision_record
{
    private:
        //vector<string> v;
        vector<string> v[SPLIT_NUM];
        int cur = 0;
        unordered_set<string> h;
        mutex mtx;
public:

    void add(string name)
    {
        lock_guard<mutex> lk(mtx);
        if (h.find(name) == h.end())
        {
            h.insert(name);
            v[cur].push_back(name);
        }
    }

    string get()
    {
        lock_guard<mutex> lk(mtx);
        string r;
        if (h.empty())
            r = -1;
        else
        {
            int b = (cur + intRand(SPLIT_NUM)) % SPLIT_NUM;
            while (v[b].empty())
                b = (b + 1) % SPLIT_NUM;
            r = v[b][intRand(static_cast<const int>(v[b].size()))];
        }
        return r;
    }

    void inc_rem()
    {
        lock_guard<mutex> lk(mtx);
        cur = (cur + 1) % SPLIT_NUM;
        for (auto n:v[cur])
            h.erase(h.find(n));
        v[cur].clear();
    }

};


#endif