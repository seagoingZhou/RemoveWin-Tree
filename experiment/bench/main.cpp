#include <cstdio>
#include <ctime>
#include <thread>
#include <condition_variable>
#include <random>
#include <sys/stat.h>

#include "generator/uniform_generator.h"
#include "hiredis/hiredis.h"

using namespace std;

const char *ips[3] = {"192.168.188.135",
                      "192.168.188.136",
                      "192.168.188.137"};

int DELAY = 50;
int DELAY_LOW = 10;
int TOTAL_SERVERS = 9;
int TOTAL_OPS = 20000000;
int OP_PER_SEC = 10000;

inline void set_default()
{
    DELAY = 50;
    DELAY_LOW = 10;
    TOTAL_SERVERS = 9;
    TOTAL_OPS = 20000000;
    OP_PER_SEC = 10000;
}

inline void set_speed(int speed)
{
    set_default();
    OP_PER_SEC = speed;
    TOTAL_OPS = 200000;
}

inline void set_replica(int replica)
{
    set_default();
    TOTAL_SERVERS = replica * 3;
    TOTAL_OPS = 20000000;
}

inline void set_delay(int hd, int ld)
{
    set_default();
    DELAY = hd;
    DELAY_LOW = ld;
    TOTAL_OPS = 10000000;
}