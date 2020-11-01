#include "server.h"
#include "RWFramework.h"
#include "crdt_set_common.h"

#define PN_SET_TABLE_SUFFIX "_pnsets_"

#ifdef PN_OVERHEAD
#define SUF_RZETOTAL "pnsetotal"
static redisDb *cur_db = NULL;
static sds cur_tname = NULL;
#endif

#ifdef COUNT_OPS
static int rcount = 0;
#endif

#ifdef SET_LOG
static FILE *pnsLog = NULL;
#define check(f)\
    do\
    {\
        if((f)==NULL)\
            (f)=fopen("pnsLog","a");\
    }while(0)
#endif

typedef struct PN_SET_element
{
    int current;
} pne;

pne *pneNew()
{
    pne *e = zmalloc(sizeof(pne));
    e->current = 0;
    return e;
}

pne *pneHTGet(redisDb *db, robj *tname, robj *key, int create)
{
    robj *ht = getInnerHT(db, tname->ptr, PN_SET_TABLE_SUFFIX, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key->ptr);
    pne *e;
    if (value == NULL)
    {
        if (!create)return NULL;
        e = pneNew();
        hashTypeSet(ht, key->ptr, sdsnewlen(&e, sizeof(pne *)), HASH_SET_TAKE_VALUE);
#ifdef PN_OVERHEAD
        inc_ovhd_count(cur_db, cur_tname, SUF_OZETOTAL, 1);
#endif
    }
    else
    {
        e = *(pne **) (value->ptr);
        decrRefCount(value);
    }
    return e;
}

void pnsaddGenericCommand(client* c, robj* setName) {
    long long added;
    int i;
    int n = c->rargc;
    getLongLongFromObject(c->rargv[n - 1], &added);
    if (added == 0) {
        return;
    }
    robj* set = getSetOrCreate(c->db, setName, c->rargv[n - 3]);
    for (i = 0; i < added; ++i) {
        int idx = n - 3 - 2 * i;
        pne* e = pneHTGet(c->db, setName, c->rargv[idx], 1);
        long long cnt;
        getLongLongFromObject(c->rargv[idx + 1], &cnt);
        e->current += cnt;
        if (e->current > 0) {
            setTypeAdd(set, c->rargv[idx]->ptr);
        }
    }
}

void pnsremGenericCommand(client* c, robj* setName) {
    long long remed;
    int i;
    int n = c->rargc;
    getLongLongFromObject(c->rargv[n - 1], &remed);
    if (remed == 0) {
        return;
    }
    robj* set = getSetOrCreate(c->db, setName, c->rargv[n - 2]);
    for (i = 0; i < remed; ++i) {
        int idx = n - 2 - i;
        pne* e = pneHTGet(c->db, setName, c->rargv[idx], 1);
        e->current += 1;
        if (e->current <= 0) {
            setTypeRemove(set, c->rargv[idx]->ptr);
        }
    }
}

void pnsaddCommand(client *c) {
    #ifdef PN_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *set;
                int j;
                long long added = 0;

                set = lookupKeyRead(c->db,c->argv[1]);
                for (j = 2; j < c->argc; j++) {
                    if (set == NULL || !setTypeIsMember(set, c->argv[j]->ptr)) {
                        ++added;
                        pne *e = oreHTGet(c->db, c->argv[1], c->argv[j], 1);
                        RARGV_ADD_SDS(c->argv[j]->ptr);
                        int cnt = 1 - e->current;
                        RARGV_ADD_SDS(sdsfromlonglong(cnt));
                    }
                }
                RARGV_ADD_SDS(sdsfromlonglong(added));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                pnsaddGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void pnsremCommand(client *c) {
    #ifdef PN_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *set;
                int j;
                long long remed = 0;

                if ((set = lookupKeyWriteOrReply(c,c->argv[1],shared.czero))) {
                    return;
                }
                for (j = 2; j < c->argc; j++) {
                    if (setTypeIsMember(set, c->argv[j]->ptr)) {
                        ++remed;
                        RARGV_ADD_SDS(c->argv[j]->ptr);
                    }
                }
                RARGV_ADD_SDS(sdsfromlonglong(remed));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                pnsremGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void pnsunionstoreCommand(client *c) {
    #ifdef PN_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj* dstset = createIntsetObject();
                robj* set = lookupKeyRead(c->db,c->argv[1]);
                sunionResult(c, c->argv + 2, c->argc - 2, dstset);
                setTypeIterator *si;
                sds ele;
                int added = 0;
                si = setTypeInitIterator(dstset);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    reh *e = rwseHTGet(c->db, c->argv[1], eleObj->ptr, 1);
                    if (set == NULL || !setTypeIsMember(set, ele)) {
                        ++added;
                        pne *e = pneHTGet(c->db, c->argv[1], eleObj, 1);
                        RARGV_ADD_SDS(sdsnew(ele));
                        int cnt = 1 - e->current;
                        RARGV_ADD_SDS(sdsfromlonglong(cnt));
                    }
                    sdsfree(ele);
                    decrRefCount(eleObj);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(added));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                pnsaddGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void pnsdiffstoreCommand(client *c) {
    #ifdef PN_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *dstset = createIntsetObject();
                robj *set = lookupKeyRead(c->db, c->argv[1]);
                sunionResult(c, c->argv + 3, c->argc - 3, dstset);
                setTypeIterator *si;
                sds ele;
                int remed = 0;
                int tagNum;
                si = setTypeInitIterator(dstset);
                while((ele = setTypeNextObject(si)) != NULL) {
                    if (setTypeIsMember(set, ele)) {
                         ++remed;
                        RARGV_ADD_SDS(sdsnew(ele));
                    }
                    sdsfree(ele);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(remed));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                pnsremGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void pnsinsterstoreCommand(client *c) {
    #ifdef PN_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *dstset = createIntsetObject();
                robj *set = lookupKeyRead(c->db, c->argv[1]);
                sinterResult(c, c->argv + 3, c->argc - 3, dstset);
                setTypeIterator *si;
                sds ele;
                int remed = 0;
                si = setTypeInitIterator(set);
                while((ele = setTypeNextObject(si)) != NULL) {
                    if (!setTypeIsMember(dstset, ele)) {
                        ++remed;
                        RARGV_ADD_SDS(sdsnew(ele));
                    }
                    sdsfree(ele);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(remed));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                pnsremGenericCommand(c, c->rargv[1]);
        CRDT_END

}