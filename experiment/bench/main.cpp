#include <cstdio>
#include <ctime>

#include "exp_runner.h"
#include "tree/tree_generator.h"
#include "tree/tree_cmd.h"
#include "set/set_cmd.h"
#include "set/set_generator.h"
#include "set/set_basic.h"

using namespace std;

const char *ips[3] = {"192.168.193.1",
                      "192.168.193.2",
                      "192.168.193.3"};



int DELAY = 20;
int DELAY_LOW = 5;
int TOTAL_SERVERS = 9;
int TOTAL_OPS = 2000000;
int OP_PER_SEC = 10000;
int ROUND = 0;

inline void set_default()
{
    DELAY = 100;
    DELAY_LOW = 20;
    TOTAL_SERVERS = 9;
    TOTAL_OPS = 500000;
    OP_PER_SEC = 5000;
}

inline void treeConfigDefault() {
    DELAY = 100;
    DELAY_LOW = 20;
    TOTAL_SERVERS = 9;
    OP_PER_SEC = 7000;
    TOTAL_OPS = OP_PER_SEC * 100;
}

inline void treeConfigDelay(int delay) {
    treeConfigDefault();
    DELAY = delay;
    DELAY_LOW = delay / 5;
}

inline void treeConfigReplica(int replica) {
    treeConfigDefault();
    TOTAL_SERVERS = 3 * replica;
}

inline void treeConfigSpeed(int speed) {
    treeConfigDefault();
    OP_PER_SEC = speed;
    TOTAL_OPS = OP_PER_SEC * 100;
}

inline void treeConfigOvhd() {
    treeConfigDefault();
    //TOTAL_OPS = OP_PER_SEC * 100;
}


inline void SetConfigDefault() {
    DELAY = 100;
    DELAY_LOW = 20;
    TOTAL_SERVERS = 9;
    OP_PER_SEC = 10000;
    TOTAL_OPS = OP_PER_SEC * 60;
}

inline void SetConfigDelay(int delay) {
    DELAY = delay;
    DELAY_LOW = delay / 5;
}

inline void SetConfigReplica(int replica) {
    TOTAL_SERVERS = 3 * replica;
}

inline void SetConfigSpeed(int speed) {
    OP_PER_SEC = speed;
    TOTAL_OPS = OP_PER_SEC * 60;
}

inline void set_speed(int speed)
{
    set_default();
    OP_PER_SEC = speed;
    TOTAL_OPS = 100 * speed;
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
    //TOTAL_OPS = 1000000;
}

/********************************************************************
 *  Tree 实验
 * ******************************************************************/


int tree_test_dis(const char *dir){
    tree_log tlog("rw",dir);
    tree_generator gen(tlog);
    tree_cmd read_members(treemembers,"","","",tlog);
    tree_cmd::setStartTime();

    exp_runner<string> runner(tlog,gen);
    runner.set_cmd_read(read_members);
    return runner.run();
}

int tree_test_ovhd(const char *dir){
    tree_log tlog("rw",dir);
    tree_generator gen(tlog);
    tree_cmd read_overhead(overhead,"","","",tlog);
    tree_cmd::setStartTime();

    exp_runner<string> runner(tlog,gen);
    runner.set_cmd_ovhd(read_overhead);
    return runner.run();
}

void tree_init(){
    redisReply *reply;
    redisContext *c = redisConnect("192.168.193.1", 6379);
    reply = (redisReply *) redisCommand(c,"treecreat t");
    freeReplyObject(reply);
    redisFree(c);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int treeReplica(int replica) {
    int ret = 0;
    treeConfigReplica(replica);
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    char pycmd0[256];
    char pycmd[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", TOTAL_SERVERS/3);
    system(pycmd0);
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    tree_init();
    ret = tree_test_dis("../result/RawData");
    if (ret == -1) {
        return -1;
    }

    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
    return ret;
}

int treeDelay(int delay) {
    int ret = 0;
    treeConfigDelay(delay);
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    char pycmd0[256];
    char pycmd[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", TOTAL_SERVERS/3);
    system(pycmd0);
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    tree_init();
    ret = tree_test_dis("../result/RawData");
    if (ret == -1) {
        return -1;
    }

    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
    return ret;
}

int test_delay(double hd, double ld) {

    int ret = 0;
    double hd_r = hd * 0.05;
    double ld_r = ld * 0.05;
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);

    set_delay(hd, ld);
    char pycmd[256];
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    tree_init();
    
    ret = tree_test_dis("../result/RawData");
    if (ret == -1) {
        return -1;
    }
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
    return ret;
}

int treeSpeed(int speed) {
    int ret = 0;
    treeConfigSpeed(speed);
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    char pycmd0[256];
    char pycmd[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", TOTAL_SERVERS/3);
    system(pycmd0);
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    tree_init();
    ret = tree_test_dis("../result/RawData");
    if (ret == -1) {
        return -1;
    }

    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
    return ret;

}

int test_speed(int speed) {
    int ret = 0;
    set_speed(speed);
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    char pycmd[256];
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    tree_init();
    
    ret = tree_test_dis("../result/RawData");
    if (ret == -1) {
        return -1;
    }
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
    return ret;
}

void treeTestDelay() {
    int hd = 500;
    int ret;
    while (hd >= 50) {
        ret = treeDelay(hd);
        if (ret == 0) {
            hd -= 50;
        }
    }
}

void treeTestSpeed() {
    int speed = 10000;
    int ret;
    while (speed >= 5000) {
        ret = treeSpeed(speed);
        if (ret == 0) {
            speed -= 1000;
        }
    }
}

void treeTestReplica() {
    int replica = 5;
    int ret = 0;
    while (replica >= 1) {
        ret = treeReplica(replica);
        if (ret == 0) {
            --replica;
        }
    }
}

void tree_experiment()
{
    treeConfigSpeed(10000);
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    TOTAL_OPS = OP_PER_SEC * 100;
    TOTAL_SERVERS = 9;
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    char pycmd0[256];
    char pycmd[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", TOTAL_SERVERS/3);
    system(pycmd0);
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    tree_init();
    tree_test_dis("../result/RawData");
    
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
}

void tree_ovhd_experiment(){
    treeConfigOvhd();
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    char pycmd0[256];
    char pycmd[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", TOTAL_SERVERS/3);
    system(pycmd0);
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    tree_init();
    tree_test_ovhd("../result/RawData");
    
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
}

/********************************************************************
 *  Set 实验
 * ******************************************************************/


int setTestDis(const char *type, const char *dir, int ssize, int ksize){
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    set_log setLog(type, dir, ssize, ksize, MAX_KEY_SIZE, MIN_KEY_SIZE, 0);
    set_generator gen(setLog);
    set_cmd read_members("",MEMBERS,"","","",setLog);
    setLog.initSet();

    char pycmd[256];
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    
    exp_runner<string> runner(setLog, gen);
    runner.set_cmd_read(read_members);
    return runner.run();
}

int setTestOvhd(const char *type, const char *dir) {
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    set_log setLog(type, dir, 1, 1000, MAX_KEY_SIZE, MIN_KEY_SIZE, 0);
    set_generator gen(setLog);
    set_cmd read_ovhd(type,OVERHEAD,"","","",setLog);
    setLog.initSet();
    char pycmd[256];
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    exp_runner<string> runner(setLog, gen);
    runner.set_cmd_ovhd(read_ovhd);
    return runner.run();

}

void setConfigOvhd() {
    SetConfigDefault();
    OP_PER_SEC = 10000;
    TOTAL_OPS = OP_PER_SEC * 100;
}

void setOverhead(const char *type) {
    setConfigOvhd();
    char pycmd0[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", 3);
    system(pycmd0);
    
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    setTestOvhd(type, "../result/RawData");
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);

    
}

void set_exp() {

    set_default();
    set_speed(1000);
    //TOTAL_OPS = 1000;
    char pycmd0[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", 3);
    system(pycmd0);
    
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    setTestDis("rw","../result/RawData", 150, 300);
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
}

int setSpeed(const char *type, int speed) {
    SetConfigDefault();
    SetConfigSpeed(speed);
    int ret = 0;

    char pycmd0[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", 3);
    system(pycmd0);
    
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    ret = setTestDis(type, "../result/RawData", 150, 500);
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);

    return ret;

}

int setDelay(const char *type, int delay) {
    int ret = 0;
    SetConfigDefault();
    SetConfigDelay(delay);
    char pycmd0[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", 3);
    system(pycmd0);

    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    ret = setTestDis(type, "../result/RawData", 150, 500);
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);

    return ret;
}

int setReplica(const char *type, int replica) {
    int ret = 0;
    SetConfigDefault();
    SetConfigReplica(replica);
    char pycmd0[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", replica);
    system(pycmd0);

    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    ret = setTestDis(type, "../result/RawData", 150, 500);
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);

    return ret;
}

void setTestSpeed(const char *type) {
    int speed = 500;
    int ret = 0;
    ROUND = 0;
    while (speed <= 2500) {
        ret = setSpeed(type, speed);
        if (ret == 0) {
            if (ROUND < 50) {
                ++ROUND;
            } else {
                speed += 500;
                ROUND = 0;
            }
            
        }
    }
}

void setTestDelay(const char *type) {
    int delay = 150;
    int ret = 0;
    ROUND = 1;
    while (delay >= 50) {
        ret = setDelay(type, delay);
        if (ret == 0) {
            if (ROUND < 50) {
                ++ROUND;
            } else {
                delay -= 25;
                if (delay == 100) {
                    delay -= 25;
                }
                ROUND = 1;
            }
            
        }
    }
}

void setTestReplica(const char *type) {
    int replica = 1;
    int ret = 0;
    ROUND = 0;
    while (replica <= 5) {
        ret = setReplica(type, replica);
        if (ret == 0) {
            if (ROUND < 50) {
                ++ROUND;
            } else {
                ++replica;
                if (replica == 3) {
                    ++replica;
                }
                ROUND = 0;
            }
            
        }
    }
}

int main(int argc, char *argv[])
{
    //time_max();
    //test_count_dis_one(ips[0],6379);
    //printf("test begin...\n");
    //tree_experiment();
    treeTestReplica();
    //treeTestSpeed();
    //treeTestDelay();
    //delayTest();
    //speedTest();
    //set_exp();
    //setTestSpeed("rw");
    //setTestSpeed("pn");
    //setTestSpeed("or");
    //setSpeed(3000);
    //setTestDelay("rw");
    //setTestDelay("pn");
    //setTestDelay("or");
    //setTestReplica("rw");
    //setTestReplica("pn");
    //setTestReplica("or");
    //setReplica("or", 1);
    //tree_ovhd_experiment();
    //setSpeed("rw", 2500);
    //setSpeed("pn", 2500);
    //setOverhead("or");
    //setOverhead("pn");
    //setOverhead("rw");
    


    return 0;
}

