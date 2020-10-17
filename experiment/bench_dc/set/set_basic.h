#ifndef TREE_BASICS_H
#define TREE_BASICS_H

#define MAX_ELE 200000
#define MAX_INIT 100
#define MAX_INCR 50

#define PR_ADD 0.2
#define PR_REM 0.2
#define PR_UNION 0.3
#define PR_INTER 0.3
#define PR_DIFF 0.3

#define PADD PR_ADD
#define PREM (PADD + PR_REM)
#define PUNION (PREM + PR_UNION)
#define PINTER (PUNION + PR_INTER)

#define PR_UNION_INTER 0.2
#define PR_UNION_DIFF 0.2
#define PR_INTER_DIFF 0.2

#define MAX_KEY_SIZE 100000000


enum op_type
{
    ADD = 0, REM = 1, UNION = 2, INTER = 3, DIFF = 4, MEMBERS = 5
};

enum set_type
{
    RW = 0, PN = 1, OR = 2
};

#endif
