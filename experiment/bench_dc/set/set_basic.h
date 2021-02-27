#ifndef SET_BASICS_H
#define SET_BASICS_H

#define MAX_ELE 200000
#define MAX_INIT 100
#define MAX_INCR 50

#define PR_SADD 0.45
#define PR_SREM 0.35
#define PR_SUNION 0.06
#define PR_SINTER 0.07
#define PR_SDIFF 0.07

#define PREM PR_SADD
#define PADD (PREM + PR_SREM)
#define PUNION (PADD + PR_SUNION)
#define PINTER (PUNION + PR_SINTER)

#define MAX_KEY_SIZE 1000
#define MIN_KEY_SIZE 10
#define REMOTE_MAX_KEY_SIZE 600
#define REMOTE_MIN_KEY_SIZE 400

#define SIMPLE 0
#define COMPLEX 1
#define MODEL COMPLEX

#define P_ADD_REM 0.5


enum set_op_type
{
    ADD = 0, REM = 1, UNION = 2, INTER = 3, DIFF = 4, MEMBERS = 5
};

enum set_type
{
    RW = 0, PN = 1, OR = 2
};

#endif
