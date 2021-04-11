#include "server.h"
#include "RWFramework.h"
#include "crdt_set_common.h"

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

typedef struct RW_SET_element {
    reh header;
} rwse;

#ifdef RWF_SET_OVERHEAD
#define RWF_SET_ELE_SIZE sizeof(rwse) + sizeof(vc) + server.p2p_count * sizeof(int)
#endif

reh *rwseNew() {
    rwse *e = zmalloc(sizeof(rwse));
    REH_INIT(e);
    return (reh *) e;
}

inline rwse *rwseHTGet(redisDb *db, robj *tname, robj *key, int create) {
    return (rwse *) rehHTGet(db, tname, RW_SET_TABLE_SUFFIX, key, create, rwseNew
#ifdef RW_OVERHEAD
            ,cur_db, cur_tname, SUF_RZETOTAL
#endif
    );
}

setTypeIterator *setTypeInitSafeIterator(robj *subject) {
    setTypeIterator *si = zmalloc(sizeof(setTypeIterator));
    si->subject = subject;
    si->encoding = subject->encoding;
    if (si->encoding == OBJ_ENCODING_HT) {
        si->di = dictGetSafeIterator(subject->ptr);
    } else if (si->encoding == OBJ_ENCODING_INTSET) {
        si->ii = 0;
    } else {
        serverPanic("Unknown set encoding");
    }
    return si;
}

void setRemoveFunc(client *c, rwse *e, vc *t, robj *setName, robj *eleName) {
    if (removeCheck((reh *) e, t)) {
        REH_RMV_FUNC(e,t);
        robj* set = getSetOrCreate(c->db, setName, eleName->ptr);
        setTypeRemove(set, eleName->ptr);
        server.dirty++;
    }
}

robj* getSetOrCreate(redisDb *db, robj *setName, sds eleSample) {
    robj* set = lookupKeyWrite(db,setName);
    if (set == NULL) {
        set = setTypeCreate(eleSample);
        dbAdd(db, setName, set);
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
    robj* set = getSetOrCreate(c->db, setName, c->rargv[n - 3]->ptr);

    for (i = 0; i < (int)added; ++i) {
        int idx = n - 3 - 2 * i;
        vc *t = CR_GET(idx + 1);
        rwse* e = rwseHTGet(c->db, setName, c->rargv[idx], 1);
        setRemoveFunc(c, e, t, setName, c->rargv[idx]);
        if (insertCheck((reh *) e, t)) {
            PID(e) = t->id;
            setTypeAdd(set, c->rargv[idx]->ptr);
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
    robj* set = getSetOrCreate(c->db, setName, c->rargv[n - 3]->ptr);
    for (i = 0; i < remed; ++i) {
        int idx = n - 3 - 2 * i;
        vc *t = CR_GET(idx + 1);
        rwse* e = rwseHTGet(c->db, setName, c->rargv[idx], 1);
        setRemoveFunc(c, e, t, setName, c->rargv[idx]);
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
                robj* set;
                set = lookupKeyRead(c->db,c->argv[1]);
                if (set != NULL && set->type != OBJ_SET) {
                    addReply(c,shared.wrongtypeerr);
                    return;
                }
                int n = c->argc;
                for (j = 2; j < n; j++) {
                    reh *e = rwseHTGet(c->db, c->argv[1], c->argv[j], 1);
                    if (!EXISTS(e)) {
                        ++added;
                        RARGV_ADD_SDS(sdsdup(c->argv[j]->ptr));
                        ADD_CR_NON_RMV(e);
                    }
                }
                RARGV_ADD_SDS(sdsfromlonglong(added));
                if (added == 0) {
                    addReply(c,shared.ele_exist);
                    return;
                }
                
    
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
                robj* set;
                long long remed = 0;
                if ((set = lookupKeyWriteOrReply(c,c->argv[1],shared.czero)) == NULL ||
                    checkType(c,set,OBJ_SET)) {

                    return;
                }
                for (j = 2; j < c->argc; j++) {
                    reh *e = rwseHTGet(c->db, c->argv[1], c->argv[j], 1);   
                    if (setTypeIsMember(set, c->argv[j]->ptr)) {
                        ++remed;
                        RARGV_ADD_SDS(sdsnew(c->argv[j]->ptr));
                        ADD_CR_RMV(e);
                    }
                }
                RARGV_ADD_SDS(sdsfromlonglong(remed));
                if (remed == 0) {
                    addReply(c,shared.ele_nexist);
                    return;
                }
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                sremGenericCommand(c, c->rargv[1]);
        CRDT_END

}


void sunionResult(client *c, robj **setkeys, int setnum, robj *dstset) {
    setTypeIterator *si;
    sds ele;
    int j;

    for (j = 0; j < setnum; j++) {
        robj *setobj = lookupKeyRead(c->db, setkeys[j]);
        if (setobj == NULL) {
            continue;
        }
        if (checkType(c, setobj, OBJ_SET)) {
            return;
        }
        si = setTypeInitIterator(setobj);
        while((ele = setTypeNextObject(si)) != NULL) {
            setTypeAdd(dstset,ele);
            sdsfree(ele);
        }
        setTypeReleaseIterator(si);
    }
}

void sdiffResult(client *c, int idx, int setnum, robj *dstset) {
    setTypeIterator *si;
    sds ele;
    robj *set = lookupKeyRead(c->db, c->argv[idx]);
    if (set == NULL) {
        return;
    }
    robj* unionSet = createIntsetObject();
    sunionResult(c, c->argv + 3, c->argc - 3, unionSet);
    si = setTypeInitIterator(set);
    while((ele = setTypeNextObject(si)) != NULL) {
        if (!setTypeIsMember(unionSet, ele)) {
            setTypeAdd(dstset,ele);
        }
        sdsfree(ele);
    }
    setTypeReleaseIterator(si);
    decrRefCount(unionSet);
}

unsigned long dictSize0(const dict* d) {
    return d->ht[0].used+d->ht[1].used;
}

unsigned long setTypeSize0(const robj *subject) {
    if (subject->encoding == OBJ_ENCODING_HT) {
        return dictSize0((const dict*)subject->ptr);
    } else if (subject->encoding == OBJ_ENCODING_INTSET) {
        return intsetLen((const intset*)subject->ptr);
    } else {
        serverPanic("Unknown set encoding");
    }
}

void sinterResult(client *c, int idx, int setnum, robj *dstset) {
    setTypeIterator *si;
    sds elesds;
    int64_t intobj;
    int j;
    int encoding;
    /*
    robj *setobj = lookupKeyRead(c->db, c->argv[idx]);
    if (setobj == NULL) {
        return;
    }
    if (checkType(c, setobj, OBJ_SET)) {
        return;
    }
    int minIdx = 0;
    int minSize = setTypeSize0(*(robj**)setobj);

    for (j = 1; j < setnum; j++) {
        setobj = lookupKeyRead(c->db, c->argv[idx + j]);
        if (setobj == NULL) {
            return;
        }
        if (checkType(c, setobj, OBJ_SET)) {
            return;
        }
        int size = setTypeSize(*(robj**)setobj);
        if (size < minIdx) {
            minIdx = j;
            minSize = size;
        }
    }
    */

    /* Iterate all the elements of the first (smallest) set, and test
     * the element against all the other sets, if at least one set does
     * not include the element it is discarded */
    robj *minsetobj = lookupKeyRead(c->db, c->argv[idx]);
    if (minsetobj == NULL) {
        return;
    }
    if (checkType(c, minsetobj, OBJ_SET)) {
        return;
    }
    si = setTypeInitSafeIterator(minsetobj);
    while((encoding = setTypeNext(si,&elesds,&intobj)) != -1) {
        for (j = 1; j < setnum; j++) {
            robj *cursetobj = lookupKeyRead(c->db, c->argv[idx + j]);
            if (cursetobj == NULL) {
                return;
            }
            if (checkType(c, cursetobj, OBJ_SET)) {
                return;
            }
            if (encoding == OBJ_ENCODING_INTSET) {
                /* intset with intset is simple... and fast */
                if (cursetobj->encoding == OBJ_ENCODING_INTSET &&
                    !intsetFind((intset*)cursetobj->ptr,intobj)) {
                    
                    break;
                /* in order to compare an integer with an object we
                 * have to use the generic function, creating an object
                 * for this */
                } else if (cursetobj->encoding == OBJ_ENCODING_HT) {
                    elesds = sdsfromlonglong(intobj);
                    if (!setTypeIsMember(cursetobj,elesds)) {
                        sdsfree(elesds);
                        break;
                    } 
                    sdsfree(elesds);
                }
            } else if (encoding == OBJ_ENCODING_HT) {
                if (!setTypeIsMember(cursetobj,elesds)) {
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
}


void rwsunionstoreCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj* set = lookupKeyRead(c->db,c->argv[1]);
                if (set == NULL) {
                    return;
                }
                robj* dstset = createIntsetObject();
                sunionResult(c, c->argv + 2, c->argc - 2, dstset);
                setTypeIterator *si;
                sds ele;
                long long added = 0;
                si = setTypeInitIterator(dstset);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    reh *e = rwseHTGet(c->db, c->argv[1], eleObj, 1);
                    if (set == NULL || !setTypeIsMember(set, ele)) {
                        ++added;                      
                        RARGV_ADD_SDS(sdsnew(ele));
                        ADD_CR_NON_RMV(e);
                    }
                    sdsfree(ele);
                    decrRefCount(eleObj);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(added));
                if (added == 0) {
                    addReply(c,shared.ele_exist);
                    return;
                }
    
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
                robj *set = lookupKeyRead(c->db, c->argv[1]);
                if (set == NULL) {
                    return;
                }
                robj *dstset = createIntsetObject();
                sdiffResult(c, 2, c->argc - 2, dstset);
                setTypeIterator *si;
                sds ele;
                long long remed = 0;
                si = setTypeInitSafeIterator(set);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    if (!setTypeIsMember(dstset, ele)) {
                        reh *e = rwseHTGet(c->db, c->argv[1], eleObj, 1);
                        ++remed;
                        RARGV_ADD_SDS(sdsnew(ele));
                        ADD_CR_RMV(e);
                    }
                    sdsfree(ele);
                    decrRefCount(eleObj);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(remed));
                if (remed == 0) {
                    addReply(c,shared.ele_nexist);
                    return;
                }
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                sremGenericCommand(c, c->rargv[1]);
        CRDT_END

}

void rwsinterstoreCommand(client *c) {
    #ifdef RW_OVERHEAD
        PRE_SET;
    #endif
        CRDT_BEGIN
            CRDT_PREPARE
                robj *dstset = createIntsetObject();
                robj *set = lookupKeyWrite(c->db, c->argv[1]);
                if (set == NULL) {
                    return;
                }
                sinterResult(c, 2, c->argc - 2, dstset);
                setTypeIterator *si;
                sds ele;
                long long remed = 0;
                si = setTypeInitIterator(set);
                while((ele = setTypeNextObject(si)) != NULL) {
                    robj *eleObj = createObject(OBJ_STRING, sdsnew(ele));
                    if (setTypeSize(dstset) == 0 || !setTypeIsMember(dstset, ele)) {
                        reh *e = rwseHTGet(c->db, c->argv[1], eleObj, 1);
                        if (EXISTS(e)) {
                            ++remed;
                            RARGV_ADD_SDS(sdsnew(ele));
                            ADD_CR_RMV(e);
                        }

                    }
                    sdsfree(ele);
                    decrRefCount(eleObj);
                }
                setTypeReleaseIterator(si);
                decrRefCount(dstset);
                RARGV_ADD_SDS(sdsfromlonglong(remed));
                if (remed == 0) {
                    addReply(c,shared.ele_nexist);
                    return;
                }
    
            CRDT_EFFECT
    #ifdef COUNT_OPS
                ocount++;
    #endif
                sremGenericCommand(c, c->rargv[1]);
        CRDT_END

}

#ifdef RWF_SET_OVERHEAD
void rwfSetOverhead(client* c) {
    robj *ht = getInnerHT(c->db, c->argv[1]->ptr, RW_SET_TABLE_SUFFIX, 0);
    if (ht == NULL) {
        addReplyLongLong(c, 0);
        return;
    }
    robj *o;
    if ((o = lookupKeyReadOrReply(c,c->argv[1],shared.czero)) == NULL ||
        checkType(c,o,OBJ_SET)) return;

    unsigned long size = hashTypeLength(ht);
    double ovhd = size * RWF_SET_ELE_SIZE;
    ovhd = ovhd * (1.0 / setTypeSize(o));
    addReplyDouble(c,ovhd);

}

#endif