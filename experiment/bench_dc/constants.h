//
// Created by user on 18-11-15.
//

#ifndef BENCH_CONSTANTS_H
#define BENCH_CONSTANTS_H

// experiment settings
//#define DELAY 50
//#define DELAY_LOW 10
//#define TOTAL_SERVERS 9
//#define OP_PER_SEC 10000
extern int DELAY;
extern int DELAY_LOW;
extern int TOTAL_SERVERS;
extern int OP_PER_SEC;

extern int TOTAL_OPS;

#define MAX_TIME_COLISION DELAY
#define SPLIT_NUM 10
#define SLP_TIME_MICRO (MAX_TIME_COLISION * 1000 / SPLIT_NUM)

#define THREAD_PER_SERVER 3

#define OP_PER_THREAD (TOTAL_OPS/TOTAL_SERVERS/THREAD_PER_SERVER)

// time in microseconds
#define INTERVAL_TIME (1000000.0*TOTAL_SERVERS*THREAD_PER_SERVER/OP_PER_SEC)
//#define OP_PS_PTH ((double)OP_PER_SEC/TOTAL_SERVERS/THREAD_PER_SERVER)
//#define ESTIMATE_COST (TOTAL_SERVERS*THREAD_PER_SERVER*(26.3629 + 0.0082*OP_PS_PTH))
//#define _SLEEP_TIME (INTERVAL_TIME-ESTIMATE_COST)
//#define SLEEP_TIME ((_SLEEP_TIME>0)?_SLEEP_TIME:0)

// time in seconds
#define TIME_OVERHEAD 1
#define TIME_MAX 1000

#define MIN_TREE_SIZE 1000

#endif //BENCH_CONSTANTS_H
