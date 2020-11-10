#include <cstdio>
#include <ctime>

#include "exp_runner.h"
#include "tree/tree_generator.h"
#include "tree/tree_cmd.h"
#include "set/set_cmd.h"

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
    DELAY = 300;
    DELAY_LOW = 60;
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
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);

    set_default();
    system("python3.6 ../redis_test/connection.py");
    tree_init();
    tree_test_dis("../result/RawData");
    
/*
    for (int i = 0; i < 30; i++)
    {
        test_delay(i);
        test_replica(i);
        test_speed(i);
    }
*/
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("total time: %f\n", time_diff_sec);
}

int main(int argc, char *argv[])
{
    //time_max();
    //test_count_dis_one(ips[0],6379);
    //tree_experiment();
    delayTest();
    //speedTest();

    return 0;
}

/*
void time_max()
{
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == nullptr || c->err)
    {
        if (c)
        {
            printf("Error: %s\n", c->errstr);
        }
        else
        {
            printf("Can't allocate redis context\n");
        }
        return;
    }
    struct timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    redisReply *reply;
    thread threads[THREAD_PER_SERVER];
    reply = static_cast<redisReply *>(redisCommand(c, "VCNEW s"));
    freeReplyObject(reply);

//    default_random_engine e;
//    uniform_int_distribution<unsigned> u(0, 4);

    for (thread &t : threads)
    {
        t = thread([] {
            redisContext *cl = redisConnect("127.0.0.1", 6379);
            redisReply *r;
            for (int i = 0; i < 10000; i++)
            {
                r = static_cast<redisReply *>(redisCommand(cl, "VCINC s"));
                freeReplyObject(r);
            }
            redisFree(cl);
        });
    }
    for (thread &t : threads)
    {
        t.join();
    }

    reply = static_cast<redisReply *>(redisCommand(c, "VCGET s"));
    printf("%s\n", reply->str);
    freeReplyObject(reply);
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (double) (t2.tv_usec - t1.tv_usec) / 1000000;
    printf("%f, %f\n", time_diff_sec, (2.0 + THREAD_PER_SERVER * 10000) / time_diff_sec);
    redisFree(c);
}

void conn_one_server(const char *ip, const int port, vector<thread *> &thds, rpq_generator &gen)
{
    for (int i = 0; i < THREAD_PER_SERVER; ++i)
    {
        auto t = new thread([ip, port, &thds, &gen] {
            redisContext *c = redisConnect(ip, port);
            for (int times = 0; times < OP_PER_THREAD; ++times)
            {
                gen.gen_and_exec(c);
                this_thread::sleep_for(chrono::microseconds((int) SLEEP_TIME));
            }
            redisFree(c);
        });
        thds.emplace_back(t);
    }
}
*/

/*
void test_local(rpq_type zt)
{
    vector<thread *> thds;
    rpq_log qlog;
    rpq_generator gen(zt, qlog);

    for (int i = 0; i < 5; ++i)
    {
        conn_one_server("127.0.0.1", 6379 + i, thds, gen);
    }

    bool mb = true, ob = true;
    thread max([&mb, &qlog, zt] {
        rpq_cmd c(zt, zmax, -1, -1, qlog);
        redisContext *cl = redisConnect("127.0.0.1", 6379);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfor-loop-analysis"
        while (mb)
        {
            this_thread::sleep_for(chrono::seconds(TIME_MAX));
            c.exec(cl);
        }
#pragma clang diagnostic pop
        redisFree(cl);
    });
    thread overhead([&ob, &qlog, zt] {
        rpq_cmd c(zt, zoverhead, -1, -1, qlog);
        redisContext *cl = redisConnect("127.0.0.1", 6381);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfor-loop-analysis"
        while (ob)
        {
            this_thread::sleep_for(chrono::seconds(TIME_OVERHEAD));
            c.exec(cl);
        }
#pragma clang diagnostic pop
        redisFree(cl);
    });
    for (auto t:thds)
        t->join();
    mb = false;
    ob = false;
    printf("ending.\n");
    max.join();
    overhead.join();
    gen.stop_and_join();
    qlog.write_file(rpq_cmd_prefix[zt]);
}
 */

/*
void test_count_dis_one(const char *ip, const int port, rpq_type zt)
{
    thread threads[THREAD_PER_SERVER];
    rpq_log qlog;
    rpq_generator gen(zt, qlog);

    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);

    for (thread &t : threads)
    {
        t = thread([&ip, &port, &gen] {
            redisContext *c = redisConnect(ip, port);
            for (int times = 0; times < 20000; ++times)
            {
                gen.gen_and_exec(c);
            }
            redisFree(c);
        });
    }

    for (thread &t : threads)
    {
        t.join();
    }

    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (double) (t2.tv_usec - t1.tv_usec) / 1000000;
    printf("%f, %f\n", time_diff_sec, 20000 * THREAD_PER_SERVER / time_diff_sec);

    redisContext *cl = redisConnect(ips[1], port);
    auto r = static_cast<redisReply *>(redisCommand(cl, "ozopcount"));
    printf("%lli\n", r->integer);
    freeReplyObject(r);
    redisFree(cl);
}
*/

/*
void rpq_test_dis(rpq_type zt, const char *dir)
{
    rpq_log qlog(rpq_cmd_prefix[zt], dir);
    rpq_generator gen(zt, qlog);
    rpq_cmd read_max(zt, zmax, -1, -1, qlog);
    rpq_cmd ovhd(zt, zoverhead, -1, -1, qlog);
    rpq_cmd opcount(zt, zopcount, -1, -1, qlog);

    exp_runner<int> runner(qlog, gen);
    runner.set_cmd_opcount(opcount);
    runner.set_cmd_ovhd(ovhd);
    runner.set_cmd_read(read_max);
    runner.run();
}

void delay_fix(int delay, int round, rpq_type type)
{
    set_delay(delay, delay / 5);
    char n[64], cmd[64];
    sprintf(n, "../result/delay/%d", round);
    sprintf(cmd, "python3 ../redis_test/connection.py %d %d %d %d", delay, delay / 5, delay / 5, delay / 25);
    system(cmd);
    rpq_test_dis(type, n);
}


void test_delay(int round)
{
    bench_mkdir("../result/delay");
    char n[64];
    sprintf(n, "../result/delay/%d", round);
    bench_mkdir(n);
    for (int d = 20; d <= 380; d += 40)
    {
        delay_fix(d, round, o);
        delay_fix(d, round, r);
    }
}

void replica_fix(int replica, int round, rpq_type type)
{
    set_replica(replica);
    char n[64], cmd[64];
    sprintf(n, "../result/replica/%d", round);
    sprintf(cmd, "python3 ../redis_test/connection.py %d", replica);
    system(cmd);
    rpq_test_dis(type, n);
}

void test_replica(int round)
{
    bench_mkdir("../result/replica");
    char n[64];
    sprintf(n, "../result/replica/%d", round);
    bench_mkdir(n);
    for (int replica:{1, 2, 3, 4, 5})
    {
        replica_fix(replica, round, o);
        replica_fix(replica, round, r);
    }
}

void speed_fix(int speed, int round, rpq_type type)
{
    system("python3 ../redis_test/connection.py");
    char n[64];
    sprintf(n, "../result/speed/%d", round);
    set_speed(speed);
    rpq_test_dis(type, n);
}

void test_speed(int round)
{
    bench_mkdir("../result/speed");
    char n[64];
    sprintf(n, "../result/speed/%d", round);
    bench_mkdir(n);
    for (int i = 500; i <= 10000; i += 100)
    {
        speed_fix(i, round, o);
        speed_fix(i, round, r);
    }
}
*/