//
// Created by admin on 2020/1/7.
//

#ifndef BENCH_EXP_RUNNER_H
#define BENCH_EXP_RUNNER_H

#include <ctime>
#include <thread>
#include <unordered_map>

#include "util.h"
#include "constants.h"

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include "../../redis-4.0.8/deps/hiredis/hiredis.h"
#include <direct.h>

#endif

using namespace std;
extern const char *ips[3];

template <class T>
class exp_runner
{
private:
    rdt_log &log;
    generator<T> &gen;
    cmd *read_cmd = nullptr;
    cmd *ovhd_cmd = nullptr;
    cmd *opcount_cmd = nullptr;

    vector<thread *> thds;
    vector<task_queue *> tasks;

    int conn_one_server_timed(const char *ip, int port) {
        volatile int ret = 0;
        for (int i = 0; i < THREAD_PER_SERVER; ++i) {
            auto task = new task_queue();
            auto t = new thread([this, ip, port, task, &ret] {
                redisContext *c = redisConnect(ip, port);
                if (c == nullptr || c->err)
                {
                    if (c) {
                        printf("Error: %s, ip:%s, port:%d\n", c->errstr, ip, port);
                    } else {
                        printf("Can't allocate redis context\n");
                    }
                    exit(-1);
                }
                for (int times = 0; times < OP_PER_THREAD; ++times) {
                    task->worker();
                    ret = gen.gen_and_exec(c);
                    if (ret == -1) {
                        break;
                    }
                }
                redisFree(c);
            });
            thds.emplace_back(t);
            tasks.emplace_back(task);
        }

        return ret;
    }

public:

    exp_runner(rdt_log &log, generator<T> &gen) : gen(gen), log(log) {}

    void set_cmd_read(cmd &readCmd)
    {
        read_cmd = &readCmd;
    }

    void set_cmd_ovhd(cmd &ovhdCmd)
    {
        ovhd_cmd = &ovhdCmd;
    }

    void set_cmd_opcount(cmd &opcountCmd)
    {
        opcount_cmd = &opcountCmd;
    }

    int run() {
        volatile int ret = 0;
        timeval t1{}, t2{};
        gettimeofday(&t1, nullptr);

        for (auto ip:ips) {
            for (int i = 0; i < (TOTAL_SERVERS / 3); ++i) {
                ret = conn_one_server_timed(ip, 6379 + i);
                if (ret == -1) {
                    return ret;
                }
            }
        }

        thread timer([this] {
            timeval td{}, tn{};
            gettimeofday(&td, nullptr);
            for (int times = 0; times < OP_PER_THREAD; ++times) {
                for (auto t:tasks) {
                    t->add();
                }
                //this_thread::sleep_for(chrono::microseconds((int)SLEEP_TIME));
                gettimeofday(&tn, nullptr);
                auto slp_time =
                        (td.tv_sec - tn.tv_sec) * 1000000 + td.tv_usec - tn.tv_usec +
                        (long) ((times + 1) * INTERVAL_TIME);
                this_thread::sleep_for(chrono::microseconds(slp_time));
            }
        });

        volatile bool rb, ob;
        thread *read_thread = nullptr, *ovhd_thread = nullptr;

        if (read_cmd!=nullptr) {
            rb = true;
            
            read_thread = new thread([this, &rb, &ret] {
                unordered_map<string, vector<redisContext *>> clientPool;
                for (auto ip:ips) {
                    vector<redisContext *> ports;
                    string keyIp = string(ip);
                    clientPool.insert({keyIp, ports});
                    for (int i = 0; i < (TOTAL_SERVERS / 3); ++i) {
                        redisContext *c = redisConnect(ip, 6379 + i);
                        if (c == nullptr || c->err) {
                            if (c) {
                                printf("Error: %s, ip:%s, port:%d\n", c->errstr, ip, 6379 + i);
                            } else {
                                printf("Can't allocate redis context\n");
                            }
                            exit(-1);
                        }
                        clientPool.at(keyIp).push_back(c);
                    }
                }
                while (rb) {
                    for (auto ip:ips) {
                        if (!rb || ret == -1) {
                            break;
                        }
                        string keyIp = string(ip);
                        for (int i = 0; i < (TOTAL_SERVERS / 3) && rb; ++i) {
                            redisContext *c1 = clientPool.at(keyIp).at(i);
                            this_thread::sleep_for(chrono::milliseconds(TIME_MAX));
                            ret = read_cmd->exec(c1);
                            if (ret == -1) {
                                break;
                            }
                        }
                    } 
                }
                for (auto ip:ips) {
                    string keyIp = string(ip);
                    for (int i = 0; i < (TOTAL_SERVERS / 3); ++i) {
                        redisContext *c1 = clientPool.at(keyIp).at(i);
                        redisFree(c1);
                    }
                }
            });
        }

        if (ovhd_cmd!=nullptr)
        {
            ob = true;
            ovhd_thread = new thread([this, &ob] {
                redisContext *cl = redisConnect(ips[1], 6379);
                while (ob)
                {
                    this_thread::sleep_for(chrono::seconds(TIME_OVERHEAD));
                    ovhd_cmd->exec(cl);
                }
                if (opcount_cmd!=nullptr)
                    opcount_cmd->exec(cl);
                redisFree(cl);
            });
            //ovhd_thread = &overhead;
        }

        timer.join();
        for (auto t:thds)
            t->join();

        gettimeofday(&t2, nullptr);
        double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
        printf("%f, %f\n", time_diff_sec, TOTAL_OPS / time_diff_sec);

        printf("ending.\n");

        if (read_thread != nullptr)
        {
            rb = false;
            read_thread->join();
            delete read_thread;
        }
        if (ovhd_thread != nullptr)
        {
            ob = false;
            ovhd_thread->join();
            delete ovhd_thread;
        }
        gen.stop_and_join();
        if (ret == 0) {
            log.write_file();
        }

        for (auto t :thds)
            delete t;
        for (auto t :tasks)
            delete t;

        this_thread::sleep_for(chrono::seconds(5));
        return ret;
    }
};

#endif //BENCH_EXP_RUNNER_H
