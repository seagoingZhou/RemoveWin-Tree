#include "server.h"
#include "RWFramework.h"

#define RW_SET_TABLE_SUFFIX "_rwsets_"

#ifdef RW_OVERHEAD
#define SUF_RZETOTAL "rwsetotal"
static redisDb *cur_db = NULL;
static sds cur_tname = NULL;
#endif

#ifdef COUNT_OPS
static int rcount = 0;
#endif

#ifdef SET_LOG
static FILE *rwsLog = NULL;
#define check(f)\
    do\
    {\
        if((f)==NULL)\
            (f)=fopen("rwsLog","a");\
    }while(0)
#endif

typedef struct RW_SET_element
{
    reh header;
    void* member;
} rwse;

reh *rwseNew()
{
    rwse *e = zmalloc(sizeof(rwse));
    REH_INIT(e);
    e->member = NULL;
    return (reh *) e;
}

inline rwse *rwseHTGet(redisDb *db, robj *tname, robj *key, int create)
{
    return (rwse *) rehHTGet(db, tname, RW_SET_TABLE_SUFFIX, key, create, rwseNew
#ifdef RW_OVERHEAD
            ,cur_db, cur_tname, SUF_RZETOTAL
#endif
    );
}

void removeFunc(client *c, rwse *e, vc *t, robj *eleName)
{
    if (removeCheck((reh *) e, t))
    {
        REH_RMV_FUNC(e,t);
        sdsfree(e->member);
        e->member = NULL;
        robj* set = getSetOrCreate(c->db, c->rargv[1], c->rargv[2]);
        setTypeRemove(set, eleName->ptr);
        server.dirty++;
    }
}

robj* getSetOrCreate(redisDb *db, robj *setName, robj *eleName) {
    robj* set = lookupKeyWrite(db,setName);
    if (set == NULL) {
        set = setTypeCreate(eleName);
        dbAdd(db,setName,set);
    }
    return set;
}

void saddGenericCommand(client* c) {
    long long added;
    int i;
    int n = c->rargc;
    robj* set = getSetOrCreate(c->db, c->rargv[1], c->rargv[2]);
    getLongLongFromObject(c->rargv[n - 1], &added);
    for (i = 0; i < added; ++i) {
        int idx = n - 2 - 2 * i;
        vc *t = CR_GET(idx);
        rwse* e = rwseHTGet(c->db, c->rargv[1], c->rargv[idx - 1], 1);
        removeFunc(c, e, t, c->rargv[idx - 1]);
        if (insertCheck((reh *) e, t)) {
            PID(e) = t->id;
            e->member = sdsnew(c->rargv[idx - 1]->ptr);
            setTypeAdd(set, c->rargv[idx - 1]->ptr);
            server.dirty++;
        }
        deleteVC(t);
    }
}

void sremGenericCommand(client* c) {
    int i;
    int n = c->rargc;
    int eleNums = (n - 2) / 2;
    robj* set = getSetOrCreate(c->db, c->rargv[1], c->rargv[2]);
    for (i = 0; i < eleNums; ++i) {
        int jdx = n - 1 - i;
        int idx = n - eleNums - 1 - i;
        vc *t = CR_GET(jdx);
        rwse* e = rwseHTGet(c->db, c->rargv[1], c->rargv[jdx], 1);
        removeFunc(c, e, t, c->rargv[jdx]);
        deleteVC(t);
    }
}


void rwsaddCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                int j;
                long long added = 0;
                for (j = 2; j < c->argc; j++) {
                    reh *e = rwseHTGet(c->db, c->argv[1], c->argv[j], 1);
                    if (!EXISTS(e)) {
                        ++added;
                        lc *t = LC_NEW(e->current);
                        e->current++;
                        RARGV_ADD_SDS(lcToSds(t));
                        zfree(t);
                    }
                }
                RARGV_ADD_SDS(sdsfromlonglong(added));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                saddGenericCommand(c);
        CRDT_END

}

void rwsremCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                int j;
                for (j = 2; j < c->argc; j++) {
                    reh *e = rwseHTGet(c->db, c->argv[1], c->argv[j], 1);   
                    lc *t = LC_NEW(e->current);
                    e->current++;
                    RARGV_ADD_SDS(lcToSds(t));
                    zfree(t);
                    
                }
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                sremGenericCommand(c);
        CRDT_END

}

void rwsunionstoreCommand(client *c) {

}

void rwsinsterstoreCommand(client *c) {

}

void rwsdiffstoreCommand(client *c) {

}