#include "server.h"
#include "RWFramework.h"
#include "lamport_clock.h"
#include "vector_clock.h"

#define RW_TREE_SUFFIX "_tree_"
#define TREE_INFO "_tree_info_"
#define TREE_ROOT "_root_"
#define UIDLENGTH 32

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

/**********************队列*****************************/
typedef struct QNode{
        sds data;
        struct QNode *next;
}QNode;

typedef struct Queue{
    QNode* front;
    QNode* rear;
}Queue;

QNode* newQNode(sds val){
    QNode* qn = (QNode*) malloc(sizeof(QNode));
    qn->data = sdsdup(val);
    qn->next = NULL;
    return qn;
}

Queue* initQueue(){
    Queue* q = (Queue*) malloc(sizeof(Queue));
    q->front = newQNode(sdsnew(""));
    q->rear = q->front;
    return q;
}

int QueueEmpty(Queue* q){
    return (q->front==q->rear)?1:0;
}

void enQueue(Queue* q, sds val){
    QNode* qn = newQNode(val);
    q->rear->next = qn;
    q->rear = q->rear->next;

    return;
}

sds deQueue(Queue* q){

    if (QueueEmpty(q)){
        return NULL;
    } 

    QNode* qn = q->front->next;
    if (qn==q->rear){
        q->rear = q->front;
    }
    sds val = sdsdup(qn->data);
    q->front->next = qn->next;

    sdsfree(qn->data);
    free(qn);
    return val;
   
}

/*******************tree node**********************/

typedef struct RAW_RT_node
{
    vc *vectorClock;
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

/****************辅助函数************************/
reh *rtnNew(){
    rtn *tn = zmalloc(sizeof(rtn));
    REH_INIT(tn);
    tn->tdata = zmalloc(sizeof(rawt));
    tn->tdata->vectorClock = l_newVC;
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

void tRemFunc(client *c, rtn *tn,sds uid,rtn *tn_p, vc *t)
{
    if (tn&&removeCheck((reh *) tn, t))
    {
        REH_RMV_FUNC(tn,t);
        if (tn_p){
            setTypeRemove(tn_p->tdata->children,uid);
        }

        /***delete ***/
        robj *tree = lookupKeyWrite(c->db,c->rargv[1]);
        setTypeRemove(tree,uid);
        
        //notifyLoop(e, c->db);
    }
}

void RemoveFun(client *c, robj* tname, robj* subtreeEles, vc *t){

     if (setTypeSize(subtreeEles) > 0){
                setTypeIterator *si;
                sds ele;
                si = setTypeInitIterator(subtreeEles);
                while((ele = setTypeNextObject(si)) != NULL) {
                    rtn* tn = rtnHTGet(c->db,tname,ele,1);
                    rtn* tn_p = NULL;
                    if (tn){
                        tn_p = rtnHTGet(c->db,tname,tn->tdata->parent,0);
                    }
                    tRemFunc(c,tn,ele,tn_p,t);
                    
                    sdsfree(ele);
                }
                setTypeReleaseIterator(si);
    }


}

robj* subTree(redisDb *db, robj* treeHT, sds uid){
    
    robj* subtree = createSetObject();
    Queue* queue = initQueue();
    enQueue(queue,sdsdup(uid));

    while(!QueueEmpty(queue)){
        sds parent_id = deQueue(queue);
        //rtn* tn = tnHTGet(db, treeHT, parent_id, 0);
        rtn* tn = rtnHTGet(db,treeHT,parent_id,0);
        if (tn && EXISTS(tn) && setTypeSize(tn->tdata->children)>0){
            sds ele;
            setTypeIterator *si;
            si = setTypeInitIterator(tn->tdata->children);
            while((ele = setTypeNextObject(si)) != NULL) {
                
                enQueue(queue,ele);
                sdsfree(ele);
            }
            setTypeReleaseIterator(si);
        }
        setTypeAdd(subtree,parent_id);      
        sdsfree(parent_id);
    }

    sdsfree(queue->front->data);
    free(queue->front);
    free(queue);

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

            robj *tree = lookupKeyRead(c->db,c->argv[1]);
            if (tree != NULL) {
                
                if (tree->type != OBJ_TREE) {
                    //addReply(c,shared.wrongtypeerr);
                    //return;
                }
                addReply(c,shared.ele_exist);
                return;
            }
            

        CRDT_EFFECT
            robj *tree ;
            tree = lookupKeyWrite(c->db,c->rargv[1]);
            if (tree == NULL){
                //创建tree
                tree = createSetObject();
                dbAdd(c->db,c->rargv[1],tree);

                /* 插入根节点 
                 * root->uid = "0,0"
                 * root->name = "_root_"
                 */
                sds uid = sdsnew("0,0");
                setTypeAdd(tree,uid);
                
                rtn* tn = rtnHTGet(c->db,c->rargv[1],uid,1);
                tn->tdata->uid = sdsdup(uid);
                tn->tdata->name = sdsnew(TREE_ROOT);
                tn->tdata->parent = sdsnew("null");
                PID(tn) = CURRENT_PID;
                
                sdsfree(uid);

                server.dirty++;
                //sdsfree(rootName);

            } 

    CRDT_END
}

//简单地展示tree
void treeMembers(client *c){

    void *replylen = NULL;
    int cardinality = 0;
    
    robj *tree = lookupKeyRead(c->db,c->argv[1]);
    if (tree == NULL) {
        addReply(c,shared.czero);
        return;
    } else if (tree->type != OBJ_TREE) {
        //addReply(c,shared.wrongtypeerr);
        //return;
    }
    
    replylen = addDeferredMultiBulkLength(c);
    
    sds uid;
    
    treeTypeIterator *tri;
    tri = treeTypeInitIterator(tree);
    
    while ((uid = treeTypeNextObject(tri)) != NULL){
        
        rtn* tn = rtnHTGet(c->db,c->argv[1],uid,0);
        if (tn && EXISTS(tn)){
            sds parent = sdsdup(tn->tdata->parent);
            sds nodename = sdscatfmt(sdsdup(uid)," %S",sdsdup(tn->tdata->name));
            
            sds output = sdscatfmt(sdsdup(nodename)," %S (%S)[",sdsdup(parent),
                                        sdsfromlonglong(setTypeSize(tn->tdata->children)));
            if (setTypeSize(tn->tdata->children)>0){
                setTypeIterator *si;
                sds ele;
                si = setTypeInitIterator(tn->tdata->children);
                while((ele = setTypeNextObject(si)) != NULL) {
                    output = sdscatfmt(output,"%S ",sdsdup(ele));
                    sdsfree(ele);
                }
                setTypeReleaseIterator(si);
            }

            output = sdscatfmt(output,"]");
            addReplyBulkCBuffer(c,output,sdslen(output));
            cardinality++;
            sdsfree(output);
            sdsfree(nodename);
        }
        
        sdsfree(uid);
    }
    treeTypeReleaseIterator(tri);

    setDeferredMultiBulkLength(c,replylen,cardinality);

}

/* 
 *  Insert treeName nodeName parent_id uid
 */
void treeNodeInsertWithUid(client *c, redisDb *db,robj *tname, robj *nodename){
    CRDT_BEGIN
        CRDT_PREPARE

            robj *tree = lookupKeyRead(c->db,c->argv[1]);
            if (tree == NULL){
                addReply(c,shared.emptymultibulk);
                return;
            } else if(tree->type != OBJ_TREE){
                //addReply(c,shared.wrongtypeerr);
                //return;
            }
         
            
           // check父节点是否存在
            
            rtn* tnp = rtnHTGet(c->db,c->argv[1],c->argv[3]->ptr,0);
            if (!tnp || !EXISTS(tnp)){
                addReply(c,shared.ele_nexist);
                return;
            }
            ADD_CR_NON_RMV(tnp);          

            //插入reh
            rtn* tn = rtnHTGet(c->db,c->argv[1],c->argv[4]->ptr,1);
            if (tn && EXISTS(tn)){
                addReply(c,shared.ele_exist);
                return;
            }
            //ADD_CR_NON_RMV(tn);
            
        CRDT_EFFECT
            /* input:   Insert  treeName  nodeName  parent_id    uid  parent_remh 
            *              0       1       2           3         4         5     
            */

            //vc *t = CR_GET_LAST;
            vc *t = CR_GET_LAST;
            robj *tree = lookupKeyWrite(c->db,c->rargv[1]);
      
            rtn* tn = rtnHTGet(c->db,c->rargv[1],c->rargv[4]->ptr,1);
            rtn* tnp = rtnHTGet(c->db,c->rargv[1],c->rargv[3]->ptr,0);         
            
            //remove function
            robj* subtreeEles = subTree(c->db,c->rargv[1],c->rargv[3]->ptr);
            setTypeAdd(subtreeEles,c->rargv[4]->ptr);
            if (tnp&&EXISTS(tnp)){
                RemoveFun(c,c->rargv[1],subtreeEles,t);
            }
            freeSetObject(subtreeEles);

            
            if (tn && tnp && EXISTS(tnp)){
                if (insertCheck((reh *) tn, t)){
                    //生成子节点
                    PID(tn) = t->id;
                    setTypeAdd(tree,c->rargv[4]->ptr);
                    
                    tn->tdata->uid = sdsdup(c->rargv[4]->ptr);
                    tn->tdata->name = sdsdup(c->rargv[2]->ptr);
                    tn->tdata->parent = sdsdup(c->rargv[3]->ptr);
                    
                    // 父节添加子节点信息
                    setTypeAdd(tnp->tdata->children,c->rargv[4]->ptr);   
                    server.dirty++;
                }
            }
            
            deleteVC(t);
           
    CRDT_END
}

/* 
 *  Delete treeName node_id
 */

void treeNodeDelete(client *c, redisDb *db,robj *tname, robj *uid){
    CRDT_BEGIN
        CRDT_PREPARE

            robj *tree = lookupKeyWrite(c->db,c->argv[1]);
            if (tree == NULL){
                addReply(c,shared.emptymultibulk);
                return;
            } else if(tree->type != OBJ_TREE){
                //addReply(c,shared.wrongtypeerr);
                //return;
            }

            // check节点是否存在
            rtn* tn = rtnHTGet(c->db,c->argv[1],c->argv[2]->ptr,0);
            if (!(tn&&EXISTS(tn))){
                addReply(c,shared.ele_nexist);
                return;
            }

            
            // 添加子节点参数
            robj* subtreeset = subTree(c->db,c->argv[1],c->argv[2]->ptr);
            sds subtree_sds = sdsnew("/");
            long long subtree_size = setTypeSize(subtreeset);
            RARGV_ADD_SDS(sdsfromlonglong(subtree_size));

            sds ele;
            setTypeIterator *si;
            si = setTypeInitIterator(subtreeset);
            while((ele = setTypeNextObject(si)) != NULL) {
                subtree_sds = sdscatfmt(subtree_sds,"%S/",sdsdup(ele));
                sdsfree(ele);
            }
            setTypeReleaseIterator(si);
            freeSetObject(subtreeset);
            RARGV_ADD_SDS(subtree_sds);   

            
            ADD_CR_RMV(tn);      

        CRDT_EFFECT
            /* 
             *   input: delete treeName node_id subtreeSize subtreeSds reh
             *             0        1      2        3           4       5
             */
            vc *t = CR_GET_LAST;

            robj* subtreeEles = subTree(c->db,c->rargv[1],c->rargv[2]->ptr);
            //setTypeRemove(subtreeEles,c->rargv[1]->ptr);
            long long len;
            sds subtreeSds = c->rargv[4]->ptr;
            char *p = subtreeSds;
            getLongLongFromObject(c->rargv[3], &len);
            char uid[UIDLENGTH];
            for (long long i=0; i<len; i++){
                p++;
                int uidPos = 0;
                while(*p!='/'){
                    uid[uidPos++] = *p;
                    p++;
                }
                uid[uidPos] = '\0';
                setTypeAdd(subtreeEles,sdsnew(uid));
            }

            rtn *tn = rtnHTGet(c->db, c->rargv[1], c->rargv[2]->ptr, 1);
            if (tn && EXISTS(tn)){
                RemoveFun(c,c->rargv[1],subtreeEles,t);
            }
            
            freeSetObject(subtreeEles);
            deleteVC(t);



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
