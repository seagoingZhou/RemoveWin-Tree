#ifndef SET_BASICS_H
#define SET_BASICS_H

#define MAX_ELE 200000
#define MAX_INIT 100
#define MAX_INCR 50
/*
#define PR_SADD 0.4
#define PR_SREM 0.3
#define PR_SUNION 0.1
#define PR_SINTER 0.1
#define PR_SDIFF 0.1
*/

#define PR_SADD 0.5
#define PR_SREM 0.5
#define PR_SUNION 0
#define PR_SINTER 0
#define PR_SDIFF 0

#define PADD PR_SADD
#define PREM (PADD + PR_SREM)
#define PUNION (PADD + PR_SUNION)
#define PINTER (PUNION + PR_SINTER)

#define MAX_KEY_SIZE 700
#define MIN_KEY_SIZE 300
#define REMOTE_MAX_KEY_SIZE 700
#define REMOTE_MIN_KEY_SIZE 300

#define SIMPLE 0
#define COMPLEX 1
#define MODEL SIMPLE
#define SIMPLE_MAX 1100
#define SIMPLE_MIN 900

#define P_ADD_REM 0.15


enum set_op_type
{
    ADD = 0, REM = 1, UNION = 2, INTER = 3, DIFF = 4, MEMBERS = 5, OVERHEAD = 6
};

enum set_type
{
    RW = 0, PN = 1, OR = 2
};

#endif
