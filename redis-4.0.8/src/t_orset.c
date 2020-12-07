#include "server.h"
#include "RWFramework.h"
#include "crdt_set_common.h"

#define OR_SET_TABLE_SUFFIX "_orsets_"

#ifdef OR_OVERHEAD
#define SUF_RZETOTAL "orsetotal"
static redisDb *cur_db = NULL;
static sds cur_tname = NULL;
#endif

#ifdef COUNT_OPS
static int rcount = 0;
#endif

#ifdef SET_LOG
static FILE *orsLog = NULL;
#define check(f)\
    do\
    {\
        if((f)==NULL)\
            (f)=fopen("orsLog","a");\
    }while(0)
#endif

typedef struct OR_SET_element
{
    int current;
    dict *aset; 
    dict *rset;
} ore;

ore *oreNew()
{
    ore *e = zmalloc(sizeof(ore));
    e->current = 0;
    e->aset = dictCreate(&setDictType, NULL);
    e->rset = dictCreate(&setDictType, NULL);
    return e;
}

sds tagGenerate(lc* tag) {
    return sdscatprintf(sdsempty(), "%d,%d", tag->id, tag->x);
}

ore *oreHTGet(redisDb *db, robj *tname, robj *key, int create)
{
    robj *ht = getInnerHT(db, tname->ptr, OR_SET_TABLE_SUFFIX, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key->ptr);
    ore *e;
    if (value == NULL)
    {
        if (!create)return NULL;
        e = oreNew();
        hashTypeSet(ht, key->ptr, sdsnewlen(&e, sizeof(ore *)), HASH_SET_TAKE_VALUE);
#ifdef RW_OVERHEAD
        inc_ovhd_count(cur_db, cur_tname, SUF_OZETOTAL, 1);
#endif
    }
    else
    {
        e = *(ore **) (value->ptr);
        decrRefCount(value);
    }
    return e;
}

inline void addTag(dict* ht, sds tag) {
    sds itag = sdsnew(tag);
    dictEntry *de = dictAddRaw(ht, itag, NULL);
    if (de) {
        dictSetKey(ht, de, itag);
        dictSetVal(ht, de, NULL);
    }

}

inline int lookup(ore* e) {
    return dictSize((const dict*)e->aset);
}

int removeTag(dict* ht, sds tag) {
    if (dictDelete(ht, tag) == DICT_OK) {
        if (htNeedsResize(ht)) {
            dictResize(ht);
        }
        return 1;
    }
    return 0;
}

void orsaddGenericCommand(client* c, robj* setName) {
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
        ore *e = oreHTGet(c->db, c->rargv[1], c->rargv[idx], 1);
        sds tag = c->rargv[idx + 1]->ptr;
        if (removeTag(e->rset, tag) == 0) {
            addTag(e->aset, tag);
        }
        if (lookup(e) > 0) {
            setTypeAdd(set, c->rargv[idx]->ptr);
        }
    }
}

void orsremGenericCommand(client* c, robj* setName) {
    long long remed;
    int i;
    int n = c->rargc;
    getLongLongFromObject(c->rargv[n - 1], &remed);
    if (remed == 0) {
        return;
    }
    
    int jdx = n - 2;
    long long tagNum = 1;
    getLongLongFromObject(c->rargv[jdx], &tagNum);
    int idx = jdx - tagNum - 1;
    robj* set = getSetOrCreate(c->db, setName, c->rargv[idx]);
    while (remed > 0) {
    
        ore *e = oreHTGet(c->db, setName, c->rargv[idx], 1);
        for (i = idx + 1; i < jdx; ++i) {
            sds tag = c->rargv[i]->ptr;
            if (removeTag(e->aset, tag) == 0) {
                addTag(e->rset, tag);
            }
        }
        
        if (lookup(e) == 0) {
            setTypeRemove(set, c->rargv[idx]->ptr);
        }
        --remed;
        if (remed > 0) {
            jdx = idx - 1;
            getLongLongFromObject(c->rargv[jdx], &tagNum);
            idx = jdx - tagNum - 1;
        }
    } 
    
}

void orsaddCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *set;
                int j;
                long long added = 0;

                set = lookupKeyRead(c->db, c->argv[1]);
                for (j = 2; j < c->argc; j++) {
                    if (set == NULL || !setTypeIsMember(set, c->argv[j]->ptr)) {
                        ++added;
                        ore *e = oreHTGet(c->db, c->argv[1], c->argv[j], 1);
                        lc *t = LC_NEW(e->current);
                        ++e->current;
                        RARGV_ADD_SDS(sdsnew(c->argv[j]->ptr));
                        RARGV_ADD_SDS(lcToSds(t));
                        zfree(t);
                    }
                }
                RARGV_ADD_SDS(sdsfromlonglong(added));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                orsaddGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void orsremCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *set;
                int j;
                long long remed = 0;
                long long tagNum;

                if ((set = lookupKeyWriteOrReply(c,c->argv[1],shared.czero)) == NULL ||
                    checkType(c,set,OBJ_SET)) {
                        
                    return;
                }
                
                for (j = 2; j < c->argc; j++) {
                    if (setTypeIsMember(set, c->argv[j]->ptr)) {
                        ++remed;
                        RARGV_ADD_SDS(sdsnew(c->argv[j]->ptr));
                        ore *e = oreHTGet(c->db, c->argv[1], c->argv[j], 1);
                        tagNum = dictSize(e->aset);
                        dictIterator *iter = dictGetSafeIterator(e->aset);
                        dictEntry *de = dictNext(iter);
                        while (de != NULL) {
                            sds tag = dictGetKey(de);
                            RARGV_ADD_SDS(sdsnew(tag));
                            de = dictNext(iter);
                        }
                        dictReleaseIterator(iter);
                        
                        RARGV_ADD_SDS(sdsfromlonglong(tagNum));
                    }
                }
                RARGV_ADD_SDS(sdsfromlonglong(remed));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                orsremGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void orsunionstoreCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj* dstset = createIntsetObject();
                robj* set = lookupKeyRead(c->db,c->argv[1]);
                sunionResult(c, c->argv + 2, c->argc - 2, dstset);
                setTypeIterator *si;
                sds ele;
                long long added = 0;
                si = setTypeInitIterator(dstset);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    ore *e = oreHTGet(c->db, c->argv[1], eleObj, 1);
                    if (set == NULL || !setTypeIsMember(set, ele)) {
                        ++added;
                        ore *e = oreHTGet(c->db, c->argv[1], eleObj, 1);
                        lc *t = LC_NEW(e->current);
                        ++e->current;
                        RARGV_ADD_SDS(sdsnew(ele));
                        RARGV_ADD_SDS(lcToSds(t));
                        zfree(t);
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
                orsaddGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void orsdiffstoreCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *dstset = createIntsetObject();
                robj *set = lookupKeyRead(c->db, c->argv[1]);
                sdiffResult(c, 2, c->argc - 2, dstset);
                setTypeIterator *si;
                sds ele;
                long long remed = 0;
                long long tagNum;
                si = setTypeInitIterator(set);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    if (!setTypeIsMember(dstset, ele)) {
                         ++remed;
                        ore *e = oreHTGet(c->db, c->argv[1], eleObj, 1);
                        tagNum = dictSize(e->aset);
                        RARGV_ADD_SDS(sdsnew(ele));
                        dictIterator *iter = dictGetSafeIterator(e->aset);
                        dictEntry *de = dictNext(iter);
                        while (de != NULL) {
                            sds tag = dictGetKey(de);
                            RARGV_ADD_SDS(sdsnew(tag));
                            de = dictNext(iter);
                        }
                        dictReleaseIterator(iter);
                        RARGV_ADD_SDS(sdsfromlonglong(tagNum));
                    }
                    sdsfree(ele);
                    decrRefCount(eleObj);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(remed));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                orsremGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void orsinterstoreCommand(client *c) {
    #ifdef PN_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *dstset = createIntsetObject();
                robj *set = lookupKeyRead(c->db, c->argv[1]);
                sinterResult(c, 2, c->argc - 2, dstset);
                setTypeIterator *si;
                sds ele;
                long long remed = 0;
                long long tagNum;
                si = setTypeInitIterator(set);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    if (!setTypeIsMember(dstset, ele)) {
                        ++remed;
                        ore *e = oreHTGet(c->db, c->argv[1], eleObj, 1);
                        tagNum = dictSize(e->aset);
                        RARGV_ADD_SDS(sdsnew(ele));
                        dictIterator *iter = dictGetSafeIterator(e->aset);
                        dictEntry *de = dictNext(iter);
                        while (de != NULL) {
                            sds tag = dictGetKey(de);
                            RARGV_ADD_SDS(sdsnew(tag));
                            de = dictNext(iter);
                        }
                        dictReleaseIterator(iter);
                        RARGV_ADD_SDS(sdsfromlonglong(tagNum));

                    }
                    sdsfree(ele);
                    decrRefCount(eleObj);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(remed));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                orsremGenericCommand(c, c->rargv[1]);
        CRDT_END

}
