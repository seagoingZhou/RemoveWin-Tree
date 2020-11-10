#ifndef TREE_BASICS_H
#define TREE_BASICS_H

#define MAX_ELE 200000
#define MAX_INIT 100
#define MAX_INCR 50

// add-rem domanient 0.6-0.1-0.3
// chg domanient 0.2-0.7-0.1
#define PR_ADD 0.6
#define PR_CHV 0.1
#define PR_REM 0.3

#define PR_ADD_R 0.15
#define PR_REM_A 0.1
#define PR_REM_R 0.1
#define PR_REM_C 0.1
#define PR_CHV_R 0.1
#define PR_CHV_C 0.1



#define PA PR_ADD
#define PC (PR_ADD + PR_CHV)

#define PAR PR_ADD_R
#define PRA PR_REM_A
#define PRR (PR_REM_A + PR_REM_R)
#define PRC (PR_REM_A + PR_REM_R + PR_REM_C)
#define PCR PR_CHV_R
#define PCC (PR_CHV_R + PR_CHV_C)


enum op_type
{
    insert = 0, del = 1, changevalue = 2, treemembers = 3
};

enum tree_type
{
    rw= 0
};

#endif
