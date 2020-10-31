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

void removeFunc(client *c, rwse *e, vc *t, robj *setName, robj *eleName) {
    if (removeCheck((reh *) e, t)) {
        REH_RMV_FUNC(e,t);
        sdsfree(e->member);
        e->member = NULL;
        robj* set = getSetOrCreate(c->db, setName, eleName->ptr);
        setTypeRemove(set, eleName->ptr);
        server.dirty++;
    }
}

robj* getSetOrCreate(redisDb *db, robj *setName, sds eleSample) {
    robj* set = lookupKeyWrite(db,setName);
    if (set == NULL) {
        set = setTypeCreate(eleSample);
        dbAdd(db,setName,set);
    }
    return set;
}

void saddGenericCommand(client* c, robj* setName) {
    long long added;
    int i;
    int n = c->rargc;
    getLongLongFromObject(c->rargv[n - 1], &added);
    if (added == 0) {
        return;
    }
    robj* set = getSetOrCreate(c->db, setName, c->rargv[n - 3]);
    for (i = 0; i < added; ++i) {
        int idx = n - 2 - 2 * i;
        vc *t = CR_GET(idx);
        rwse* e = rwseHTGet(c->db, setName, c->rargv[idx - 1], 1);
        removeFunc(c, e, t, setName, c->rargv[idx - 1]);
        if (insertCheck((reh *) e, t)) {
            PID(e) = t->id;
            e->member = sdsnew(c->rargv[idx - 1]->ptr);
            setTypeAdd(set, c->rargv[idx - 1]->ptr);
            server.dirty++;
        }
        deleteVC(t);
    }
}

void sremGenericCommand(client* c, robj* setName) {
    long long remed;
    int i;
    int n = c->rargc;
    getLongLongFromObject(c->rargv[n - 1], &remed);
    if (remed == 0) {
        return;
    }
    robj* set = getSetOrCreate(c->db, setName, c->rargv[n - 3]);
    for (i = 0; i < remed; ++i) {
        int idx = n - 2 - 2 * i;
        vc *t = CR_GET(idx);
        rwse* e = rwseHTGet(c->db, setName, c->rargv[idx - 1], 1);
        removeFunc(c, e, t, setName, c->rargv[idx - 1]);
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
                        RARGV_ADD_SDS(c->argv[j]->ptr);
                        ADD_CR_NON_RMV(e);
                    }
                }
                RARGV_ADD_SDS(sdsfromlonglong(added));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                saddGenericCommand(c, c->rargv[1]);
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
                    RARGV_ADD_SDS(c->argv[j]->ptr);
                    ADD_CR_RMV(e);
                    
                }
                RARGV_ADD_SDS(sdsfromlonglong(c->argc - 2));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                sremGenericCommand(c, c->rargv[1]);
        CRDT_END

}

int qsortCompareSetsByCardinality(const void *s1, const void *s2) {
    if (setTypeSize(*(robj**)s1) > setTypeSize(*(robj**)s2)) return 1;
    if (setTypeSize(*(robj**)s1) < setTypeSize(*(robj**)s2)) return -1;
    return 0;
}

/* This is used by SDIFF and in this case we can receive NULL that should
 * be handled as empty sets. */
int qsortCompareSetsByRevCardinality(const void *s1, const void *s2) {
    robj *o1 = *(robj**)s1, *o2 = *(robj**)s2;
    unsigned long first = o1 ? setTypeSize(o1) : 0;
    unsigned long second = o2 ? setTypeSize(o2) : 0;

    if (first < second) return 1;
    if (first > second) return -1;
    return 0;
}

void sunionResult(client *c, robj **setkeys, int setnum, robj *dstset) {
    robj **sets = zmalloc(sizeof(robj*)*setnum);
    setTypeIterator *si;
    sds ele;
    int j;

    for (j = 0; j < setnum; j++) {
        robj *setobj = lookupKeyRead(c->db, setkeys[j]);
        if (!setobj) {
            sets[j] = NULL;
            continue;
        }
        if (checkType(c, setobj, OBJ_SET)) {
            zfree(sets);
            return;
        }
        sets[j] = setobj;
    }
    /* Union is trivial, just add every element of every set to the
        * temporary set. */
    for (j = 0; j < setnum; j++) {
        if (!sets[j]) continue; /* non existing keys are like empty sets */
        si = setTypeInitIterator(sets[j]);
        while((ele = setTypeNextObject(si)) != NULL) {
            setTypeAdd(dstset,ele);
            sdsfree(ele);
        }
        setTypeReleaseIterator(si);
    }
    zfree(sets);
}

void sinterResult(client *c, robj **setkeys, int setnum, robj *dstset) {
    robj **sets = zmalloc(sizeof(robj*)*setnum);
    setTypeIterator *si;
    sds elesds;
    int64_t intobj;
    int j;
    int encoding;

    for (j = 0; j < setnum; j++) {
        robj *setobj = lookupKeyRead(c->db, setkeys[j]);
        if (!setobj) {
            sets[j] = NULL;
            continue;
        }
        if (checkType(c, setobj, OBJ_SET)) {
            zfree(sets);
            return;
        }
        sets[j] = setobj;
    }
    /* Sort sets from the smallest to largest, this will improve our
     * algorithm's performance */
    qsort(sets,setnum,sizeof(robj*),qsortCompareSetsByCardinality);

    /* Iterate all the elements of the first (smallest) set, and test
     * the element against all the other sets, if at least one set does
     * not include the element it is discarded */
    si = setTypeInitIterator(sets[0]);
    while((encoding = setTypeNext(si,&elesds,&intobj)) != -1) {
        for (j = 1; j < setnum; j++) {
            if (sets[j] == sets[0]) continue;
            if (encoding == OBJ_ENCODING_INTSET) {
                /* intset with intset is simple... and fast */
                if (sets[j]->encoding == OBJ_ENCODING_INTSET &&
                    !intsetFind((intset*)sets[j]->ptr,intobj))
                {
                    break;
                /* in order to compare an integer with an object we
                 * have to use the generic function, creating an object
                 * for this */
                } else if (sets[j]->encoding == OBJ_ENCODING_HT) {
                    elesds = sdsfromlonglong(intobj);
                    if (!setTypeIsMember(sets[j],elesds)) {
                        sdsfree(elesds);
                        break;
                    } 
                    sdsfree(elesds);
                }
            } else if (encoding == OBJ_ENCODING_HT) {
                if (!setTypeIsMember(sets[j],elesds)) {
                    break;
                }
            }
        }

        /* Only take action when all sets contain the member */
        if (j == setnum) { 
            if (encoding == OBJ_ENCODING_INTSET) {
                elesds = sdsfromlonglong(intobj);
                setTypeAdd(dstset,elesds);
                sdsfree(elesds);
            } else {
                setTypeAdd(dstset,elesds);
            } 
        }
    }
    setTypeReleaseIterator(si);
    zfree(sets);
}


void rwsunionstoreCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj* dstset = createIntsetObject();
                sunionResult(c, c->argv + 2, c->argc - 2, dstset);
                setTypeIterator *si;
                sds ele;
                int added = 0;
                si = setTypeInitIterator(dstset);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    reh *e = rwseHTGet(c->db, c->argv[1], eleObj->ptr, 1);
                    if (!EXISTS(e)) {
                        ++added;                      
                        RARGV_ADD_SDS(sdsnew(ele));
                        ADD_CR_NON_RMV(e);
                    }
                    sdsfree(ele);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(added));
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                saddGenericCommand(c, c->rargv[1]);
        CRDT_END

}


void rwsdiffstoreCommand(client *c) {
    #ifdef RW_OVERHEAD
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
                si = setTypeInitIterator(dstset);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    if (setTypeIsMember(set, ele)) {
                        reh *e = rwseHTGet(c->db, c->argv[1], eleObj->ptr, 1);
                        if (EXISTS(e)) {
                            ++remed;
                            RARGV_ADD_SDS(sdsnew(ele));
                            ADD_CR_RMV(e);
                        }

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
                sremGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void rwsinsterstoreCommand(client *c) {
    #ifdef RW_OVERHEAD
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
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    if (!setTypeIsMember(dstset, ele)) {
                        reh *e = rwseHTGet(c->db, c->argv[1], eleObj->ptr, 1);
                        if (EXISTS(e)) {
                            ++remed;
                            RARGV_ADD_SDS(sdsnew(ele));
                            ADD_CR_RMV(e);
                        }

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
                sremGenericCommand(c, c->rargv[1]);
        CRDT_END

}