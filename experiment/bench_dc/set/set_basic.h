#ifndef SET_BASICS_H
#define SET_BASICS_H

#define MAX_ELE 200000
#define MAX_INIT 100
#define MAX_INCR 50

#define PR_SADD 0.2
#define PR_SREM 0.2
#define PR_SUNION 0.3
#define PR_SINTER 0.3
#define PR_SDIFF 0.3

#define PADD PR_SADD
#define PREM (PADD + PR_SREM)
#define PUNION (PREM + PR_SUNION)
#define PINTER (PUNION + PR_SINTER)

#define PR_UNION_INTER 0.2
#define PR_UNION_DIFF 0.2
#define PR_INTER_DIFF 0.2

#define MAX_KEY_SIZE 100000000


enum set_op_type
{
    ADD = 0, REM = 1, UNION = 2, INTER = 3, DIFF = 4, MEMBERS = 5
};

enum set_type
{
    RW = 0, PN = 1, OR = 2
};

#endif
