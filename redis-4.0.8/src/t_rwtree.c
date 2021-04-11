#include "server.h"
#include "RWFramework.h"
#include "lamport_clock.h"
#include "vector_clock.h"

#define RW_TREE_SUFFIX "_tree_"
#define TREE_INFO "_tree_info_"
#define TREE_ROOT "_root_"
#define ROOT_ID "0,0"
#define UIDLENGTH 32

#ifdef TREE_OVERHEAD
long long ovhd_cnt = 0;
#endif

#ifdef RWT_OVERHEAD
#define SUF_RZETOTAL "rwttotal"
static redisDb *cur_db = NULL;
static sds cur_tname = NULL;
#endif

#ifdef RWT_COUNT_OPS
static int rcount = 0;
#endif

#ifdef RWT_LOG
static FILE *rwtLog = NULL;
#define check(f)\
    do\
    {\
        if((f)==NULL)\
            (f)=fopen("rwtlog","a");\
    }while(0)
#endif


/*******************tree node**********************/

typedef struct RAW_RT_node
{
    vc *vectorClock;
    vc *moveVC;
    void* uid;
    void* name;
    void* parent;
    robj *children; 
}rawt;

typedef struct TreeNode
{
    reh header;
    rawt* tdata;
}rtn;

#define RWF_NODE_BASE(e) (sizeof(rtn) + sizeof(rawt) + 2*sizeof(vc) + sizeof(robj))
long long treeNodeAttribute(rtn *e) {
    long long ret = 0;
    if (e->tdata->uid != NULL) {
        ret += 2 * sdslen(e->tdata->uid);
    }
    if (e->tdata->name != NULL) {
        ret += sdslen(e->tdata->name);
    }
    if (e->tdata->parent != NULL) {
        ret += sdslen(e->tdata->parent);
    }
    return ret;
}
#define RWF_NODE_SIZE(e) RWF_NODE_BASE(e) + treeNodeAttribute(e)


/****************辅助函数************************/
reh *rtnNew(){
    rtn *tn = zmalloc(sizeof(rtn));
    REH_INIT(tn);
    tn->tdata = zmalloc(sizeof(rawt));
    tn->tdata->vectorClock = l_newVC;
    tn->tdata->moveVC = l_newVC;
    tn->tdata->uid = NULL;
    tn->tdata->name = NULL;
    tn->tdata->parent = NULL;
    tn->tdata->children = createSetObject();

    return (reh *) tn;
}

inline rtn* rtnHTGet(redisDb *db, robj *tname, sds key, int create)
{
    return (rtn *) rehHTGet(db, tname, RW_TREE_SUFFIX, createObject(OBJ_STRING,sdsnew(key)), create, rtnNew
#ifdef RWT_OVERHEAD
            ,cur_db, cur_tname, SUF_RZETOTAL
#endif
    );
}

void tRemFunc(client *c, rtn *tn, vc *t)
{
    if (tn&&removeCheck((reh *) tn, t))
    {
        rtn* tn_p = rtnHTGet(c->db,c->rargv[1],tn->tdata->parent,1);
        REH_RMV_FUNC(tn,t);
        if (tn_p){
            setTypeRemove(tn_p->tdata->children,tn->tdata->uid);
        }
        sdsfree(tn->tdata->parent);
        tn->tdata->parent = sdsnew("none");
        
        //notifyLoop(e, c->db);
    }
}
int istreeMember(client* c, robj* tname,const sds id){
    sds ele = sdsnew(id);
    sds root = sdsnew(ROOT_ID);
    int res = 1;
    while(sdscmp(ele,root)!=0){
        rtn* tn = rtnHTGet(c->db,tname,ele,0);
        if(!(tn&&EXISTS(tn))){
            res = 0;
            break;
        }
        sdsfree(ele);
        ele = sdsdup(tn->tdata->parent);
    }
    sdsfree(ele);
    sdsfree(root);
    return res;
}

robj* subTree(redisDb *db, robj* treeHT, sds uid){
    
    robj* subtree = createSetObject();
    //Queue* queue = initQueue();
    //enQueue(queue,sdsdup(uid));

    list *list = listCreate();
    listAddNodeTail(list, sdsnew(uid));
    while (list->len != 0) {
        sds curId = (sds)list->head->value;
        listDelNode(list, list->head);
        rtn* tn = rtnHTGet(db,treeHT,curId,0);
        if (tn && EXISTS(tn)){
            setTypeAdd(subtree,curId);
            if (setTypeSize(tn->tdata->children)>0){
                sds ele;
                setTypeIterator *si;
                si = setTypeInitIterator(tn->tdata->children);
                while((ele = setTypeNextObject(si)) != NULL) {
                    listAddNodeTail(list, ele);
                }
                setTypeReleaseIterator(si);
            }
            
        }
        sdsfree(curId);
    }

   listEmpty(list);

    return subtree;
}

/****tree的迭代器*****/
typedef struct treeTypeIterator{
    robj* subject;
    dictIterator *di;
}treeTypeIterator;

treeTypeIterator *treeTypeInitIterator(robj *subject){
    treeTypeIterator *tri = zmalloc(sizeof(treeTypeIterator));
    tri->subject = subject;
    tri->di = dictGetIterator(subject->ptr);

    return tri;
}

void treeTypeReleaseIterator(treeTypeIterator *tri){
    dictReleaseIterator(tri->di);
    zfree(tri);
}

int treeTypeNext(treeTypeIterator *tri, sds *sdsele){
    
    dictEntry *de = dictNext(tri->di);
    if (de == NULL) return 0;
    *sdsele = dictGetKey(de);
    return 1;
}

sds treeTypeNextObject(treeTypeIterator *tri){
    sds sdsele;
    if (treeTypeNext(tri,&sdsele)){
        return sdsdup(sdsele);
    }
    return NULL;
}

int treeTypeAdd(robj* tName, sds nodeName){
    if (tName->encoding == OBJ_ENCODING_HT){
        dict *ht = tName->ptr;
        dictEntry *de = dictAddRaw(ht,nodeName,NULL);
        if (de){
            dictSetKey(ht,de,sdsdup(nodeName));
            dictSetVal(ht,de,NULL);
            return 1;
        }
    }
    return 0;
}

int treeTypeRemove(robj* tName, sds nodeName){
    if (tName->encoding == OBJ_ENCODING_HT) {
        if (dictDelete(tName->ptr,nodeName) == DICT_OK) {
            if (htNeedsResize(tName->ptr)) dictResize(tName->ptr);
            return 1;
        }
    }

    return 0;
}



/********************************/

/*
 *   CreateTree treeName 
 */
void CreateTree(client *c){
    CRDT_BEGIN
        CRDT_PREPARE

            robj *tree = getInnerHT(c->db,c->argv[1]->ptr,RW_TREE_SUFFIX,0);
            if (tree != NULL) {
                addReply(c,shared.ele_exist);
                return;
            }
            

        CRDT_EFFECT

                /* 插入根节点 
                 * root->uid = "0,0"
                 * root->name = "_root_"
                 */
                sds uid = sdsnew(ROOT_ID);
                rtn* tn = rtnHTGet(c->db,c->rargv[1],uid,1);
                tn->tdata->uid = sdsdup(uid);
                tn->tdata->name = sdsnew(TREE_ROOT);
                tn->tdata->parent = sdsnew("null");
                PID(tn) = CURRENT_PID;
                sdsfree(uid);

                server.dirty++;
                //sdsfree(rootName);

            

    CRDT_END
}

//简单地展示tree
void treeMembers(client *c){

    void *replylen = NULL;
    int cardinality = 0;
    
    robj *tree = subTree(c->db,c->argv[1],sdsnew(ROOT_ID));
    if (tree == NULL) {
        addReply(c,shared.czero);
        return;
    } 
    
    replylen = addDeferredMultiBulkLength(c);
    
    sds uid;
    
    treeTypeIterator *tri;
    tri = treeTypeInitIterator(tree);
    
    while ((uid = treeTypeNextObject(tri)) != NULL){
        
        rtn* tn = rtnHTGet(c->db,c->argv[1],uid,0);
        rtn* tn_p = rtnHTGet(c->db,c->argv[1],tn->tdata->parent,0);
        if (tn && EXISTS(tn)&&tn_p&&EXISTS(tn_p)){
            sds parent = sdsnew(tn->tdata->parent);
            sds nodename = sdscatfmt(sdsnew(uid)," %S",sdsnew(tn->tdata->name));
            
            sds output = sdscatfmt(sdsnew(nodename)," %S (%S)[",sdsnew(parent),
                                        sdsfromlonglong(setTypeSize(tn->tdata->children)));
            
            if (setTypeSize(tn->tdata->children)>0){
                setTypeIterator *si;
                sds ele;
                si = setTypeInitIterator(tn->tdata->children);
                while((ele = setTypeNextObject(si)) != NULL) {
                    output = sdscatfmt(output,"%S ",sdsnew(ele));
                    sdsfree(ele);
                }
                setTypeReleaseIterator(si);
            }

            output = sdscatfmt(output,"]");
            addReplyBulkCBuffer(c,output,sdslen(output));
            cardinality++;
            sdsfree(output);
            sdsfree(parent);
            sdsfree(nodename);
        }
        
        sdsfree(uid);
    }
    treeTypeReleaseIterator(tri);
    decrRefCount(tree);

    setDeferredMultiBulkLength(c,replylen,cardinality);

}

int checkInitVC(const vc *c){
    for (int i = 0; i < c->size; ++i)
        if (c->vector[i]>0)
            return 0;
    return 1;
}
/* 
 *  Insert treeName nodeName parent_id uid
 *    0      1          2       3        4
 */
void treeNodeInsertWithUid(client *c, redisDb *db,robj *tname, robj *nodename){
    CRDT_BEGIN
        CRDT_PREPARE

            robj *tree = getInnerHT(c->db,c->argv[1]->ptr,RW_TREE_SUFFIX,0);
            if (tree == NULL) {
                addReply(c,shared.emptymultibulk);
                return;
            }

           // check父节点是否存在 
           if (!istreeMember(c,c->argv[1],c->argv[3]->ptr)) {
                addReply(c,shared.ele_nexist);
                return;
           }        
            rtn* tnp = rtnHTGet(c->db,c->argv[1],c->argv[3]->ptr,0);
            ADD_CR_NON_RMV(tnp);          

            //插入reh
            if (istreeMember(c,c->argv[1],c->argv[4]->ptr)) {
                addReply(c,shared.ele_exist);
                return;
           } 
            rtn* tn = rtnHTGet(c->db,c->argv[1],c->argv[4]->ptr,1);
            ADD_CR_NON_RMV(tn);
            
        CRDT_EFFECT
            /* input:   Insert  treeName  nodeName  parent_id    uid  parent_remh  node_reh
            *              0       1       2           3         4         5          6    
            */

            vc *r_p = CR_GET(5);
            vc *r = CR_GET_LAST;
      
            rtn* tn = rtnHTGet(c->db,c->rargv[1],c->rargv[4]->ptr,1);
            rtn* tnp = rtnHTGet(c->db,c->rargv[1],c->rargv[3]->ptr,1);  
            tn->tdata->uid = sdsdup(c->rargv[4]->ptr);
            tn->tdata->parent = sdsdup(c->rargv[3]->ptr);
            if (checkInitVC(tn->tdata->vectorClock)){
                tn->tdata->name = sdsdup(c->rargv[2]->ptr);
            }
            // 父节添加子节点信息
            setTypeAdd(tnp->tdata->children,c->rargv[4]->ptr);

            tRemFunc(c,tnp,r_p);
            tRemFunc(c,tn,r);

            if (insertCheck((reh *) tn, r)){
                //生成子节点
                PID(tn) = r->id;    
                server.dirty++;
            }

            #ifdef TREE_OVERHEAD
            long long inc = RWF_NODE_SIZE(tn);
            ovhd_cnt += inc;
            #endif
            
            deleteVC(r);
            deleteVC(r_p);
           
    CRDT_END
}

/* 
 *  Delete treeName node_id
 */

void treeNodeDelete(client *c, redisDb *db,robj *tname, robj *uid){
    CRDT_BEGIN
        CRDT_PREPARE

            robj *tree = getInnerHT(c->db,c->argv[1]->ptr,RW_TREE_SUFFIX,0);
            if (tree == NULL) {
                addReply(c,shared.emptymultibulk);
                return;
            }

            // check节点是否存在
            if (!istreeMember(c,c->argv[1],c->argv[2]->ptr)) {
                addReply(c,shared.ele_nexist);
                return;
           }  
            rtn* tn = rtnHTGet(c->db,c->argv[1],c->argv[2]->ptr,0);
            ADD_CR_RMV(tn);      

        CRDT_EFFECT
            /* 
             *   input: delete treeName node_id   reh
             *             0        1      2        3 
             */
            vc *r = CR_GET_LAST;

            rtn *tn = rtnHTGet(c->db, c->rargv[1], c->rargv[2]->ptr, 1);
            tRemFunc(c,tn,r);
            deleteVC(r);



    CRDT_END
}

int checkCurrency(const vc * vc1, const vc *vc2){
    serverAssert(vc1->size == vc2->size);
    int flag1 = 0;
    int flag2 = 0;
    if (equalVC(vc1,vc2)) return 0;

    for (int i = 0; i < vc1->size; ++i){
        if (vc1->vector[i] > vc2->vector[i]){
            flag1 = 1;
        } else if (vc1->vector[i] < vc2->vector[i]){
            flag2 = 1;
        }
    }

    return flag1&&flag2;
}

int currentCompare(const vc * vc1, const vc *vc2) {
    serverAssert(vc1->size == vc2->size);

    for (int i = 0; i < vc1->size; ++i){
        if (vc1->vector[i] > vc2->vector[i]){
            return 1;
        } else if (vc1->vector[i] < vc2->vector[i]){
            return -1;
        }
    }

    return 0;
}

/* 
 *  changeValue treeName node_id nodeName
 */

void treeNodeChangeValue(client *c, redisDb *db,robj *tname, robj *uid){
    CRDT_BEGIN
        CRDT_PREPARE

            robj *tree = getInnerHT(c->db,c->argv[1]->ptr,RW_TREE_SUFFIX,0);
            if (tree == NULL) {
                addReply(c,shared.emptymultibulk);
                return;
            }

           // check节点是否存在
           if (!istreeMember(c,c->argv[1],c->argv[2]->ptr)) {
                addReply(c,shared.ele_nexist);
                return;
           }  
            rtn* tn = rtnHTGet(c->db,c->argv[1],c->argv[2]->ptr,0);
            
            RARGV_ADD_SDS(nowVC(tn->tdata->vectorClock));
            ADD_CR_NON_RMV(tn);

        CRDT_EFFECT
            /* 
            *  changeValue treeName node_id nodeName vc reh
            *     0            1       2        3     4   5 
            */

            vc *r = CR_GET_LAST;
            vc *vc_changeval = CR_GET(4);
           
            rtn* tn = rtnHTGet(c->db,c->rargv[1],c->rargv[2]->ptr,1);

            tRemFunc(c,tn,r);

            if (updateCheck((reh *) tn, r)){
                
                if (currentCompare(tn->tdata->vectorClock, vc_changeval) < 0) {
                    sdsfree(tn->tdata->name);
                    tn->tdata->name = sdsdup(c->rargv[3]->ptr);
                } else if (causally_ready(tn->tdata->vectorClock,vc_changeval)){
                    sdsfree(tn->tdata->name);
                    tn->tdata->name = sdsdup(c->rargv[3]->ptr);
                }
            }
            
            server.dirty++;
            updateVC(tn->tdata->vectorClock,vc_changeval);
            deleteVC(r);
            deleteVC(vc_changeval);
            
    CRDT_END
}

/*
 * treemove treename id_dst id_src
 */
void treeNodeMove(client *c, redisDb *db,robj *tname){
    CRDT_BEGIN
        CRDT_PREPARE

            robj *tree = getInnerHT(c->db,c->argv[1]->ptr,RW_TREE_SUFFIX,0);
            if (tree == NULL) {
                addReply(c,shared.emptymultibulk);
                return;
            }

            // check节点是否存在
           if (!istreeMember(c,c->argv[1],c->argv[2]->ptr)) {
                addReply(c,shared.ele_nexist);
                return;
           }
           if (!istreeMember(c,c->argv[1],c->argv[3]->ptr)) {
                addReply(c,shared.ele_nexist);
                return;
           }
           robj *src_st = subTree(c->db,c->argv[1],c->argv[3]->ptr);
           if (setTypeIsMember(src_st,c->argv[2]->ptr)){
                addReply(c,shared.err);
                return;
           }
           decrRefCount(src_st);

           rtn* tn_dst = rtnHTGet(c->db,c->argv[1],c->argv[2]->ptr,0);
           rtn* tn_src = rtnHTGet(c->db,c->argv[1],c->argv[3]->ptr,0);

           RARGV_ADD_SDS(nowVC(tn_src->tdata->moveVC));
           ADD_CR_NON_RMV(tn_dst);
           ADD_CR_NON_RMV(tn_src);

        CRDT_EFFECT
        /*
         *  treemove treename id_dst id_src move_vc dst_reh src_reh
         *      0       1        2      3      4       5      6
         */
        
            vc *r_src = CR_GET_LAST;
            vc *r_dst = CR_GET(5);
            vc *move_vc = CR_GET(4);

            rtn* tn_dst = rtnHTGet(c->db,c->rargv[1],c->rargv[2]->ptr,1);
            rtn* tn_src = rtnHTGet(c->db,c->rargv[1],c->rargv[3]->ptr,1); 

            tRemFunc(c,tn_dst,r_dst);
            tRemFunc(c,tn_src,r_src);

            if (updateCheck((reh *)tn_src,r_src)){
                if (checkCurrency(tn_src->tdata->moveVC,move_vc)){
                    sds tmp = sdsnew(tn_src->tdata->parent);
                    if (sdscmp(tmp,c->rargv[2]->ptr)>0){
                        rtn* tn_p = rtnHTGet(c->db,c->rargv[1],tmp,0);
                        if (tn_p){
                            setTypeRemove(tn_p->tdata->children,tn_src->tdata->uid);
                        }
                        setTypeAdd(tn_dst->tdata->children,tn_src->tdata->uid);
                        sdsfree(tn_src->tdata->parent);
                        tn_src->tdata->parent = sdsnew(c->rargv[2]->ptr);  
                    }
                    sdsfree(tmp);
                    
                } else if (causally_ready(tn_src->tdata->moveVC,move_vc)){
                    sds tmp = sdsnew(tn_src->tdata->parent);
                    rtn* tn_p = rtnHTGet(c->db,c->rargv[1],tmp,1);
                    if (tn_p){
                        setTypeRemove(tn_p->tdata->children,tn_src->tdata->uid);
                    }
                    setTypeAdd(tn_dst->tdata->children,tn_src->tdata->uid);
                    sdsfree(tn_src->tdata->parent);
                    tn_src->tdata->parent = sdsnew(c->rargv[2]->ptr); 
                    sdsfree(tmp);
                }
            }
            updateVC(tn_src->tdata->moveVC,move_vc);
            deleteVC(r_dst);
            deleteVC(r_src);

    CRDT_END


}

/***********************API接口函数*************************/

void treeMembersCommand(client *c){
        treeMembers(c);
}

void CreateTreeCommand(client *c){
        CreateTree(c);
}

void InsertWithIdCommand(client *c){
    treeNodeInsertWithUid(c,c->db,c->argv[1],c->argv[2]);
}


void DeleteCommand(client *c){
    treeNodeDelete(c,c->db,c->argv[1],c->argv[2]);
}

void ChangeValueCommand(client *c){
    treeNodeChangeValue(c,c->db,c->argv[1],c->argv[2]);
}

void MoveCommand(client* c){
    treeNodeMove(c,c->db,c->argv[1]);
}

/* rwftreeovhd tname*/
#ifdef TREE_OVERHEAD
void treeOverhead(client* c) {
    robj *tree = subTree(c->db,c->argv[1],sdsnew(ROOT_ID));
    if (tree == NULL) {
        addReply(c,shared.czero);
        return;
    } 
    
    long long  keySize= setTypeSize(tree);
    if (keySize == 0) {
        addReply(c,shared.czero);
        return;
    }
    long long avgOvhd = ovhd_cnt / keySize;
    decrRefCount(tree);
    addReplyLongLong(c, avgOvhd);
}
#endif