#include "server.h"
#include "RWFramework.h"
#include "lamport_clock.h"
#include "vector_clock.h"




typedef struct  TreeNode{
    reh *rem;
    vc *vectorClock;
    
    void* uid;
    void* name;
    void* parent;
    robj *children; 
    //reh *mv;
    //int renameOp;
}TreeNode;

typedef struct TreeInfo{
    lc *LamportClock;
    

}TreeInfo;

#define RW_TREE_SUFFIX "_tree_"
#define TREE_INFO "_tree_info_"
#define TREE_ROOT "_root_"
#define UIDLENGTH 32

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
    q->front = NULL;
    q->rear = NULL;
    return q;
}

int QueueEmpty(Queue* q){
    return (q->front==NULL)?1:0;
}

void enQueue(Queue* q, sds val){
    QNode* qn = newQNode(val);
    if (QueueEmpty(q)){
        q->front = qn;
        q->rear = qn;
    } else {
        q->rear->next = qn;
        q->rear = qn;
    }

    return;
}

sds deQueue(Queue* q){

    if (QueueEmpty(q)){
        return NULL;
    } 

    QNode* qn = q->front;
    sds val = sdsdup(qn->data);
    q->front = q->front->next;
    if (q->front == NULL){
        q->rear = NULL;
    }
    sdsfree(qn->data);
    free(qn);
    return val;
   
}


/***********************辅助函数*****************************/

TreeNode *tnNew(){
    
    TreeNode *tn = zmalloc(sizeof(TreeNode));
    
    tn->rem = zmalloc(sizeof(reh));
    REH_INIT(tn->rem);
    tn->vectorClock = l_newVC;
    tn->uid = NULL;
    tn->name = NULL;
    tn->parent = NULL;
    tn->children = createSetObject();
    //reh *mv;
    //int renameOp;

    //zfree(id);

    return (TreeNode*)tn;
}

TreeInfo *tinfoNew(){
    TreeInfo *ti = zmalloc(sizeof(TreeInfo));
    ti->LamportClock = LC_NEW(0);
    return ti;
}

//tree的迭代器
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

sds getUidFromTreeNodeName(sds tnName){
    char *p = tnName;
    int tid_len = 0;
    while(*p != ':'){
        tid_len++;
        p++;
    }
    
    return sdsnewlen(tnName,tid_len);
}

sds getTreeObj(robj* tree, sds uid){
    sds ret = NULL;
    sds ele;
    treeTypeIterator *tri;
    tri = treeTypeInitIterator(tree);
    while ((ele = treeTypeNextObject(tri)) != NULL){
        sds tid = getUidFromTreeNodeName(ele);
        
        if (!sdscmp(tid,uid)){
            ret = ele;
            sdsfree(ele);
            sdsfree(tid);
            break;
        }
        sdsfree(ele);
        sdsfree(tid);
    }
    treeTypeReleaseIterator(tri);

    return ret;
}





inline TreeNode *tnHTGet(redisDb *db, robj *tname, sds key, int create)
{
    robj *ht = getInnerHT(db, tname->ptr, RW_TREE_SUFFIX, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key);
    TreeNode *e;
    if (value == NULL){
        if (!create)return NULL;
        e = tnNew();
        hashTypeSet(ht, sdsdup(key), sdsnewlen(&e, sizeof(TreeNode *)), HASH_SET_TAKE_VALUE);
    } else {
        e = *(TreeNode **) (value->ptr);
        decrRefCount(value);
    }
    return e;
}

inline TreeInfo *tinfoHTGet(redisDb *db, robj *tname, sds key, int create)
{
    robj *ht = getInnerHT(db, tname->ptr, RW_TREE_SUFFIX, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key);
    TreeInfo *e;
    if (value == NULL){
        if (!create)return NULL;
        e = tinfoNew();
        hashTypeSet(ht, sdsdup(key), sdsnewlen(&e, sizeof(TreeInfo *)), HASH_SET_TAKE_VALUE);
    } else {
        e = *(TreeInfo **) (value->ptr);
        decrRefCount(value);
    }
    return e;
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



int treeTypeIsMember(robj* tree, sds uid){
    return dictFind((dict*)tree->ptr,uid) != NULL;
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
        
        TreeNode* tn = tnHTGet(c->db,c->argv[1],uid,0);
        if (tn && EXISTS(tn->rem)){
            sds parent = tn->parent;
            sds nodename = sdscatfmt(sdsdup(uid)," %S",sdsdup(tn->name));
            
            sds output = sdscatfmt(sdsdup(nodename)," %S (%S)[",sdsdup(parent),
                                        sdsfromlonglong(setTypeSize(tn->children)));
            if (setTypeSize(tn->children)>0){
                setTypeIterator *si;
                sds ele;
                si = setTypeInitIterator(tn->children);
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

robj* subTree(redisDb *db, robj* treeHT, sds uid){
    
    robj* subtree = createSetObject();
    Queue* queue = initQueue();
    enQueue(queue,sdsdup(uid));

    while(!QueueEmpty(queue)){
        sds parent_id = deQueue(queue);
        TreeNode* tn = tnHTGet(db, treeHT, parent_id, 0);
        if (tn && EXISTS(tn->rem) && setTypeSize(tn->children)>0){
            sds ele;
            setTypeIterator *si;
            si = setTypeInitIterator(tn->children);
            while((ele = setTypeNextObject(si)) != NULL) {
                
                enQueue(queue,ele);
                sdsfree(ele);
            }
            setTypeReleaseIterator(si);
        }
        setTypeAdd(subtree,parent_id);      
        sdsfree(parent_id);
    }

    free(queue);

    return subtree;
}


/*
 *   subTree treeName uid
 */
void showsubtree(client* c){
    void *replylen = NULL;
    int cardinality = 0;
    robj* subtreeset = subTree(c->db,c->argv[1],c->argv[2]->ptr);
    //TreeNode* tn = tnHTGet(c->db,c->argv[1],c->argv[2],0);
    //robj* subtreeset = tn->children;
    
    if (setTypeSize(subtreeset) == 0){
        addReply(c,shared.czero);
        return;
    }

    replylen = addDeferredMultiBulkLength(c);

    sds ele;
    setTypeIterator* si;
    si = setTypeInitIterator(subtreeset);
    while((ele = setTypeNextObject(si)) != NULL) {
        addReplyBulkCBuffer(c,ele,sdslen(ele));
        cardinality++;    
        sdsfree(ele);
    }
    setTypeReleaseIterator(si);

    setDeferredMultiBulkLength(c,replylen,cardinality);

}

/*
 *   CreateTree treeName 
 */
void CreateTree(client *c){
    CRDT_BEGIN
        CRDT_PREPARE

            robj *tree = lookupKeyWrite(c->db,c->argv[1]);
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

                // Info节点添加          
                TreeInfo *ti = tinfoHTGet(c->db,c->rargv[1],TREE_INFO,1);
                if (ti==NULL){
                    addReply(c,shared.err);
                    return;
                }

                /* 插入根节点 
                 * root->uid = "0,0"
                 * root->name = "0,0:_root_"
                 */
                sds uid = sdsnew("0,0");
                setTypeAdd(tree,uid);
                TreeNode* tn = tnHTGet(c->db,c->rargv[1],uid,1);
                tn->uid = sdsdup(uid);
                tn->name = sdsnew(TREE_ROOT);
                tn->parent = sdsnew("null");
                PID(tn->rem) = CURRENT_PID;
                sdsfree(uid);

                server.dirty++;
                //sdsfree(rootName);

            } 

    CRDT_END
}

/* 
 *  Insert treeName nodeName parent_id
 */
void treeNodeInsert(client *c, redisDb *db,robj *tname, robj *nodename){
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
         
            //robj* info = createObject(OBJ_STRING,sdsnew(TREE_INFO));
            TreeInfo* ti = tinfoHTGet(c->db,c->argv[1],TREE_INFO,0);
            if (ti==NULL){
                addReply(c,shared.err);
                return;
            }
            // check节点是否存在
            TreeNode* tnp = tnHTGet(c->db,c->argv[1],c->argv[3]->ptr,0);
            if (!(tnp&&EXISTS(tnp->rem))){
                addReply(c,shared.ele_nexist);
                return;
            }
            
                 
            //生成uid
            sds uid = lc_now(ti->LamportClock);
            lc_update(ti->LamportClock,sdsToLc(uid));
            RARGV_ADD_SDS(uid);
            TreeNode* tn = tnHTGet(c->db,c->argv[1],uid,1);
            if (EXISTS(tn->rem)){
                addReply(c,shared.ele_exist);
                return;
            }
                
            //插入reh
            ADD_CR_NON_RMV(tnp->rem);         
            ADD_CR_NON_RMV(tn->rem);
            

        CRDT_EFFECT
            /* input:   Insert  treeName  nodeName  parent_id    uid  parent_remh  rem_h
            *              0       1       2           3         4         5      6    
            */

            vc *t = CR_GET_LAST;
            vc *tp = CR_GET(5);
            robj *tree = lookupKeyWrite(c->db,c->rargv[1]);
      
            TreeNode* tn = tnHTGet(c->db,c->rargv[1],c->rargv[4]->ptr,1);
            TreeNode* tnp = tnHTGet(c->db,c->rargv[1],c->rargv[3]->ptr,0);         
            
            //remove function
            robj* subtreeEles = subTree(c->db,c->rargv[1],c->rargv[3]->ptr);
            setTypeAdd(subtreeEles,c->rargv[4]->ptr);
            subtreeRemoveFunc(c,c->rargv[1],c->rargv[3]->ptr,
                                sdsdup(tnp->parent),tp,subtreeEles);
            freeSetObject(subtreeEles);

            
            if (tnp && EXISTS(tnp->rem)){
                if (insertCheck((reh *) tn->rem, t)){
                    //生成子节点
                    PID(tn->rem) = t->id;
                    setTypeAdd(tree,c->rargv[4]->ptr);
                    
                    tn->uid = sdsdup(c->rargv[4]->ptr);
                    tn->name = sdsdup(c->rargv[2]->ptr);
                    tn->parent = sdsdup(c->rargv[3]->ptr);
                    
                    // 父节添加子节点信息
                    setTypeAdd(tnp->children,c->rargv[4]->ptr); 

                    server.dirty++;  
                    
                }
            }
            
            deleteVC(tp);
            deleteVC(t);
           
    CRDT_END
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
            
            TreeNode* tnp = tnHTGet(c->db,c->argv[1],c->argv[3]->ptr,0);
            if (!tnp || !EXISTS(tnp->rem)){
                addReply(c,shared.ele_nexist);
                return;
            }
            ADD_CR_NON_RMV(tnp->rem);          

            //插入reh
            TreeNode* tn = tnHTGet(c->db,c->argv[1],c->argv[4]->ptr,1);
            if (tn && EXISTS(tn->rem)){
                addReply(c,shared.ele_exist);
                return;
            }
            ADD_CR_NON_RMV(tn->rem);
            
        CRDT_EFFECT
            /* input:   Insert  treeName  nodeName  parent_id    uid  parent_remh  rem_h
            *              0       1       2           3         4         5      6    
            */

            vc *t = CR_GET_LAST;
            vc *tp = CR_GET(5);
            robj *tree = lookupKeyWrite(c->db,c->rargv[1]);
      
            TreeNode* tn = tnHTGet(c->db,c->rargv[1],c->rargv[4]->ptr,1);
            TreeNode* tnp = tnHTGet(c->db,c->rargv[1],c->rargv[3]->ptr,0);         
            
            //remove function
            robj* subtreeEles = subTree(c->db,c->rargv[1],c->rargv[3]->ptr);
            setTypeAdd(subtreeEles,c->rargv[4]->ptr);
            if (tn){
                subtreeRemoveFunc(c,c->rargv[1],c->rargv[3]->ptr,
                                sdsdup(tnp->parent),tp,subtreeEles);
            }
            freeSetObject(subtreeEles);

            
            if (tn && tnp && EXISTS(tnp->rem)){
                if (insertCheck((reh *) tn->rem, t)){
                    //生成子节点
                    PID(tn->rem) = t->id;
                    setTypeAdd(tree,c->rargv[4]->ptr);
                    
                    tn->uid = sdsdup(c->rargv[4]->ptr);
                    tn->name = sdsdup(c->rargv[2]->ptr);
                    tn->parent = sdsdup(c->rargv[3]->ptr);
                    
                    // 父节添加子节点信息
                    setTypeAdd(tnp->children,c->rargv[4]->ptr);   
                    server.dirty++;
                }
            }
            
            deleteVC(tp);
            deleteVC(t);
           
    CRDT_END
}



void subtreeRemoveFunc(client *c, robj* tname, sds uid, 
                            sds parent_id, vc *t, robj* subtreeEles){
    
    TreeNode* tn = tnHTGet(c->db,tname,uid,0);
    robj* tree = lookupKeyWrite(c->db,tname);
    if (tn && removeCheck(tn->rem, t)){
        REH_RMV_FUNC(tn->rem,t);
        
        sdsfree(tn->parent);
        tn->parent = sdsnew("null");
        //treeTypeRemove(tree,tn->uid);

        if (setTypeSize(subtreeEles) > 0){
                setTypeIterator *si;
                sds ele;
                si = setTypeInitIterator(subtreeEles);
                while((ele = setTypeNextObject(si)) != NULL) {
                    if (!sdscmp(ele,uid)) continue;
                    TreeNode* tn = tnHTGet(c->db,tname,ele,0);
                    if (tn){
                        PID(tn->rem) = -1;
                        TreeNode *tn_p = tnHTGet(c->db, tname, tn->parent,0);
                        if (tn_p){
                            setTypeRemove(tn_p->children,ele);
                        }
                        sdsfree(tn->parent);
                        tn->parent = sdsnew("null");
                        //increaseVC(CURRENT(tn->rem), t->id);
                        //REH_RMV_FUNC(tn->rem,);
                        //treeTypeRemove(tree,tn->uid);
                        
                    }
                    sdsfree(ele);
                }
                setTypeReleaseIterator(si);
            }

        TreeNode *tn_p = tnHTGet(c->db, tname, parent_id,0);
        if (tn_p){
            setTypeRemove(tn_p->children,uid);
        }
        
        
        server.dirty++;
    }
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
            TreeNode* tn = tnHTGet(c->db,c->argv[1],c->argv[2]->ptr,0);
            if (!(tn&&EXISTS(tn->rem))){
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

            
            ADD_CR_RMV(tn->rem);      

        CRDT_EFFECT
            /* 
             *   input: delete treeName node_id subtreeSize subtreeSds reh
             *             0        1      2        3           4       5
             */
            vc *rem = CR_GET_LAST;

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

            TreeNode *tn = tnHTGet(c->db, c->rargv[1], c->rargv[2]->ptr, 0);
            if (tn){
                subtreeRemoveFunc(c,c->rargv[1],c->rargv[2]->ptr,sdsdup(tn->parent),
                                        rem,subtreeEles);
            }
            
            freeSetObject(subtreeEles);
            deleteVC(rem);


/*
            TreeNode *tnode = tnHTGet(c->db, c->rargv[1], c->rargv[2]->ptr, 0);
            sds node_parent = sdsdup(tnode->parent);
            

            if(removeTreeNodeFunc(c,tnode,t)){

                if (setTypeSize(subtreeEles) > 0){
                    robj* tree = lookupKeyWrite(c->db,c->rargv[1]);
                    setTypeIterator *si;
                    sds ele;
                    si = setTypeInitIterator(subtreeEles);
                    while((ele = setTypeNextObject(si)) != NULL) {
                        TreeNode* tn = tnHTGet(c->db,c->rargv[1],ele,0);
                        if (tn){
                            PID(tn->rem) = -1;
                            treeTypeRemove(tree,tn->uid);
                            
                        }
                        sdsfree(ele);
                    }
                    setTypeReleaseIterator(si);
                }

                TreeNode *tn_p = tnHTGet(c->db, c->rargv[1], node_parent,0);
                setTypeRemove(tn_p->children,c->rargv[2]->ptr);
            }     
*/
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

/* 
 *  changeValue treeName node_id nodeName
 */

void treeNodeChangeValue(client *c, redisDb *db,robj *tname, robj *uid){
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

            //添加vc和reh参数
            TreeNode* tn = tnHTGet(c->db,c->argv[1],c->argv[2]->ptr,0);
            if (!(tn && EXISTS(tn->rem))){
                addReply(c,shared.ele_nexist);
                return;
            }
            RARGV_ADD_SDS(nowVC(tn->vectorClock));
            ADD_CR_NON_RMV(tn->rem);
            

        CRDT_EFFECT
            /* 
            *  changeValue treeName node_id nodeName vc reh
            */

            vc *rem = CR_GET_LAST;
            vc *vc_changeval = SdsToVC(c->rargv[c->rargc-2]->ptr);
            robj *tree = lookupKeyWrite(c->db,c->rargv[1]);
            TreeNode* tn = tnHTGet(c->db,c->rargv[1],c->rargv[2]->ptr,0);
            
            //remove function
            robj* subtreeEles = subTree(c->db,c->rargv[1],c->rargv[2]->ptr);
            if (tn){
                subtreeRemoveFunc(c,c->rargv[1],c->rargv[2]->ptr,sdsdup(tn->parent),
                                        rem,subtreeEles);
            }
            freeSetObject(subtreeEles);

            if (checkCurrency(tn->vectorClock,vc_changeval)){
                
                if (sdscmp(tn->name,c->rargv[3]->ptr)>0){
                    sdsfree(tn->name);
                    tn->name = sdsdup(c->rargv[3]->ptr);  
                }
                
            } else if (causally_ready(tn->vectorClock,vc_changeval)){
                sdsfree(tn->name);
                tn->name = sdsdup(c->rargv[3]->ptr);
            }

            server.dirty++;

            updateVC(tn->vectorClock,vc_changeval);

            deleteVC(rem);
            deleteVC(vc_changeval);
            
    CRDT_END
}

/* 
 *  move treeName dst_id src_id
 */

void treeMove(client *c,redisDb *db,robj *tname, robj *dst_id, robj* src_id){
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


            robj* subtreeEles_src = subTree(db,tname,src_id->ptr);
            if (setTypeIsMember(subtreeEles_src,dst_id->ptr)){
                addReply(c,shared.err);
                return;
            }


            //添加reh参数         
            TreeNode* dst_tn = tnHTGet(db,tname,dst_id->ptr,0);
            if (!(dst_tn && EXISTS(dst_tn->rem))){
                addReply(c,shared.ele_nexist);
                return;
            }
            ADD_CR_NON_RMV(dst_tn->rem);

            TreeNode* src_tn = tnHTGet(db,tname,src_id->ptr,0);
            if (!(src_tn && EXISTS(src_tn->rem))){
                addReply(c,shared.ele_nexist);
                return;
            }
            ADD_CR_NON_RMV(src_tn->rem);

        CRDT_EFFECT
            /* 
            *  move treeName dst_id src_id dst_rem src_rem
            *    0     1        2      3      4        5
            */
            vc *dst_rem = CR_GET(c->rargc-2);
            vc *src_rem = CR_GET_LAST;
            robj *tree = lookupKeyWrite(c->db,c->rargv[1]);
            TreeNode* dst_tn = tnHTGet(c->db,c->rargv[1],c->rargv[2]->ptr,0);
            TreeNode* src_tn = tnHTGet(c->db,c->rargv[1],c->rargv[3]->ptr,0);

            //remove function
            robj* subtreeEles_dst = subTree(c->db,c->rargv[1],c->rargv[2]->ptr);
            subtreeRemoveFunc(c,c->rargv[1],c->rargv[2]->ptr,sdsdup(dst_tn->parent),
                                        dst_rem,subtreeEles_dst);
            

            robj* subtreeEles_src = subTree(c->db,c->rargv[1],c->rargv[3]->ptr);
            subtreeRemoveFunc(c,c->rargv[1],c->rargv[3]->ptr,sdsdup(src_tn->parent),
                                        src_rem,subtreeEles_src);

            if (setTypeIsMember(subtreeEles_src,c->rargv[2]->ptr)){
                addReply(c,shared.err);
                return;
            }

            
            if (dst_tn==NULL || !EXISTS(dst_tn->rem)){
                setTypeIterator *si;
                sds ele;
                si = setTypeInitIterator(subtreeEles_src);
                while((ele = setTypeNextObject(si)) != NULL) {
                    TreeNode* tn = tnHTGet(c->db,c->rargv[1],ele,0);
                    if (tn){
                        PID(tn->rem) = -1;
                        TreeNode* tn_p = tnHTGet(c->db,c->rargv[1],tn->parent,0);
                        if (tn_p){
                            setTypeRemove(tn_p->children,ele);
                        }
                        //treeTypeRemove(tree,tn->uid);
                        
                    }
                    sdsfree(ele);
                }
                setTypeReleaseIterator(si);

                server.dirty++;
                
            } else{
                if (src_tn&&EXISTS(src_tn->rem)){

                    TreeNode *src_tn_p = tnHTGet(c->db, c->rargv[1], src_tn->parent,0);
                    if (src_tn_p){
                        setTypeRemove(src_tn_p->children,c->rargv[3]->ptr);
                    }
                    setTypeAdd(dst_tn->children,c->rargv[3]->ptr);
                    sdsfree(src_tn->parent);
                    src_tn->parent = sdsdup(c->rargv[2]->ptr);

                    server.dirty++;
                }
            }

            freeSetObject(subtreeEles_dst);
            freeSetObject(subtreeEles_src);
            deleteVC(dst_rem);
            deleteVC(src_rem);

    CRDT_END
}


/***********************API接口函数*************************/

void treeMembersCommand(client *c){
        treeMembers(c);
}

/*
 *   subTree treeName uid
 */
void subtreeCommand(client *c){
    showsubtree(c);
}


void CreateTreeCommand(client *c){
        CreateTree(c);
}

void InsertCommand(client *c){
    treeNodeInsert(c,c->db,c->argv[1],c->argv[2]);
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


void MoveCommand(client *c){
    treeMove(c,c->db,c->argv[1],c->argv[2],c->argv[3]);
}