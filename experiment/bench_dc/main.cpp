#include <cstdio>
#include <ctime>

#include "exp_runner.h"
#include "tree/tree_generator.h"
#include "tree/tree_cmd.h"
#include "set/set_cmd.h"
#include "set/set_generator.h"
#include "set/set_basic.h"

using namespace std;

const char *ips[3] = {"192.168.192.1",
                      "192.168.192.2",
                      "192.168.192.3"};



int DELAY = 20;
int DELAY_LOW = 5;
int TOTAL_SERVERS = 9;
int TOTAL_OPS = 2000000;
int OP_PER_SEC = 10000;
int ROUND = 0;

inline void set_default()
{
    DELAY = 150;
    DELAY_LOW = 30;
    TOTAL_SERVERS = 9;
    TOTAL_OPS = 500000;
    OP_PER_SEC = 5000;
    ROUND = 0;
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


int tree_test_dis(const char *dir){
    tree_log tlog("rw",dir);
    tree_generator gen(tlog);
    tree_cmd read_members(treemembers,"","","",tlog);
    tree_cmd::setStartTime();

    exp_runner<string> runner(tlog,gen);
    runner.set_cmd_read(read_members);
    return runner.run();
}

void tree_init(){
    redisReply *reply;
    redisContext *c = redisConnect("192.168.192.1", 6379);
    reply = (redisReply *) redisCommand(c,"treecreat t");
    freeReplyObject(reply);
    redisFree(c);
    std::this_thread::sleep_for(std::chrono::seconds(2));
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

void delayTest() {
    int hd = 50;
    double ld;
    int ret;
    while (hd <= 500) {
        ld = hd * 0.2;
        ret = test_delay((double)hd, ld);
        if (ret == 0) {
            hd += 50;
        }
    }
}

void speedTest() {
    int speed = 500;
    int ret;
    while (speed <= 6000) {
        ret = test_speed(speed);
        if (ret == 0) {
            speed += 500;
        }
    }
}

void tree_experiment()
{
    set_default();
    set_speed(10000);
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    TOTAL_SERVERS = 3;
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    char pycmd0[256];
    char pycmd[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", 1);
    system(pycmd0);
    sprintf(pycmd, "python3.6 ../redis_test/connection.py %f %f %f %f", hd, hd_r, ld, ld_r);
    system(pycmd);
    tree_init();
    tree_test_dis("../result/RawData");
    
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
}

int setTestDis(const char *dir, int ssize, int ksize){
    double hd = DELAY;
    double ld = DELAY_LOW;
    double hd_r = DELAY * 0.05;
    double ld_r = DELAY_LOW * 0.05;
    set_log setLog("rw",dir, ssize, ksize, MAX_KEY_SIZE, MIN_KEY_SIZE);
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

void set_exp() {

    set_default();
    set_speed(1000);
    //TOTAL_OPS = 1000;
    char pycmd0[256];
    sprintf(pycmd0, "python3.6 ../redis_test/connection.py %d", 3);
    system(pycmd0);
    
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    setTestDis("../result/RawData", 150, 300);
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
}

int main(int argc, char *argv[])
{
    //time_max();
    //test_count_dis_one(ips[0],6379);
    //tree_experiment();
    //delayTest();
    //speedTest();
    set_exp();

    return 0;
}

