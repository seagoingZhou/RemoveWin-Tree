//
// Created by user on 19-10-4.
//
#include "RWFramework.h"

robj *getInnerHT(redisDb *db, sds tname, const char *suffix, int create)
{
    robj *htname = createObject(OBJ_STRING, sdscat(sdsdup(tname), suffix));
    robj *ht = lookupKeyRead(db, htname);
    if (create && ht == NULL)
    {
        ht = createHashObject();
        dbAdd(db, htname, ht);
    }
    decrRefCount(htname);
    return ht;
}

reh *rehHTGet(redisDb *db, robj *tname, const char *suffix, robj *key, int create, reh *(*p)()
#ifdef RW_OVERHEAD
        ,redisDb *cur_db, sds cur_tname, const char *ovhd_suf
#endif
)
{
    robj *ht = getInnerHT(db, tname->ptr, suffix, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key->ptr);
    reh *e;
    if (value == NULL)
    {
        if (!create)return NULL;
        e = (*p)();
        hashTypeSet(ht, key->ptr, sdsnewlen(&e, sizeof(reh *)), HASH_SET_TAKE_VALUE);
#ifdef RW_OVERHEAD
        inc_ovhd_count(cur_db, cur_tname, ovhd_suf, 1);
#endif
    }
    else
    {
        e = *(reh **) (value->ptr);
        decrRefCount(value);
    }
    return e;
}