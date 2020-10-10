#include <cstdio>
#include <cstring>

#include "exp_setting.h"
#include "exp_env.h"
#include "exp_runner.h"

#include "rpq/rpq_generator.h"
#include "tree/tree_generator.h"

using namespace std;

/*
const char *ips[3] = {"192.168.188.135",
                      "192.168.188.136",
                      "192.168.188.137"};

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

void rpq_test_dis(rpq_type zt)
{
    rpq_log qlog(rpq_cmd_prefix[static_cast<int>(zt)]);
    rpq_generator gen(zt, qlog);
    rpq_cmd read_max(zt, rpq_op_type::max, -1, -1, qlog);
    rpq_cmd ovhd(zt, rpq_op_type::overhead, -1, -1, qlog);
    rpq_cmd opcount(zt, rpq_op_type::opcount, -1, -1, qlog);

    exp_runner<int> runner(qlog, gen);
    runner.set_cmd_opcount(opcount);
    runner.set_cmd_ovhd(ovhd);
    runner.set_cmd_read(read_max);
    runner.run();
}

void rwtree_test_dis(rwtree_type zt){
    tree_log tlog("rw");
    tree_generator gen(zt,tlog);
    tree_cmd read_members(rwtree_op_type::treemembers,"","","",tlog);

    exp_runner<string> runner(tlog, gen);
    runner.set_cmd_read(read_members);
    runner.run();

}

void delay_fix(int delay, int round, rpq_type type)
{
    exp_setting::set_delay(round, delay, delay / 5);
    
    rpq_test_dis(type);
}


void test_delay(int round)
{
    for (int d = 20; d <= 380; d += 40)
    {
        delay_fix(d, round, rpq_type::o);
        delay_fix(d, round, rpq_type::r);
    }
}

void replica_fix(int s_p_c, int round, rpq_type type)
{
    exp_setting::set_replica(round, 3, s_p_c);
    
    rpq_test_dis(type);
}

void test_replica(int round)
{
    for (int replica:{1, 2, 3, 4, 5})
    {
        replica_fix(replica, round, rpq_type::o);
        replica_fix(replica, round, rpq_type::r);
    }
}

void speed_fix(int speed, int round, rpq_type type)
{
    // system("python3 ../redis_test/connection.py");
    exp_setting::set_speed(round, speed);
    rpq_test_dis(type);
}

void test_speed(int round)
{
    for (int i = 500; i <= 10000; i += 100)
    {
        speed_fix(i, round, rpq_type::o);
        speed_fix(i, round, rpq_type::r);
    }
}

void rpq_experiment()
{
    auto start = chrono::steady_clock::now();

    exp_setting::set_pattern("ardominant");
    // system("python3 ../redis_test/connection.py");
    rpq_test_dis(rpq_type::o);
    // system("python3 ../redis_test/connection.py");
    rpq_test_dis(rpq_type::r);

    for (int i = 0; i < 30; i++)
    {
        test_delay(i);
        test_replica(i);
        test_speed(i);
    }

    auto end = chrono::steady_clock::now();
    auto time = chrono::duration_cast<chrono::duration<double> >(end - start).count();
    printf("total time: %f seconds\n", time);
}


void rwtree_experiment(){
    auto start = chrono::steady_clock::now();

    rwtree_test_dis(rwtree_type::rw);

    auto end = chrono::steady_clock::now();
    auto time = chrono::duration_cast<chrono::duration<double> >(end - start).count();
    printf("total time: %f seconds\n", time);

}

int main(int argc, char *argv[])
{
    //time_max();
    //test_count_dis_one(ips[0],6379);

    if (argc == 2)
        strcpy(exp_env::sudo_pwd, argv[1]);
    else if (argc == 1)
    {
        printf("please enter the password for sudo: ");
        scanf("%s", exp_env::sudo_pwd);
    }
    else
    {
        printf("error. too many input arguments\n");
        return -1;
    }

    rwtree_experiment();

    return 0;
}