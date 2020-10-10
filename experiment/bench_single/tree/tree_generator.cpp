#include "tree_generator.h"

void tree_generator::gen_and_exec(redis_client &c)
{
    rwtree_op_type t;
    string pid;
    string uid;
    string value;
    
    double rand = decide();
    if (ele.tSize()<MIN_TREE_SIZE||rand <= PA)
    {
        t = insert;
        //d = gen_initial();
        value = gen_value();
        uid = gen_uid();
        double conf = decide();
        
        if (conf < PAR)
        {
            pid = rem.get("0,0");
            add.add(uid);
        }
        else
        {
            pid = ele.random_insert_get();
            add.add(uid);
        }
    }
    else if (rand <= PC)
    {
        t = changevalue;
        value = gen_value();
        double conf = decide();

        if (conf < PCC){
            pid = change.get("0,0");
            change.add(pid);
        } 
        else 
        {
            pid = ele.random_get();
            change.add(pid);
        }
    }
    else
    {
        //tree的规模过小则不能执行删除操作
        if (ele.tSize()<MIN_TREE_SIZE){
            return;
        }
        t = del;
        double conf = decide();
        if (conf < PRA)
        {
            pid = add.get("0,0");
            if (pid == "0,0")
            {
                pid = ele.random_get();
                if (pid == "0,0")return;
            }
            
            rem.add(pid);
            
            
        }
        else if (conf < PRR)
        {
            pid = rem.get("0,0");
            if (pid == "0,0")
            {
                pid = ele.random_get();
                if (pid == "0,0")return;
                
                rem.add(pid);
                
            }
        }
        else if (conf < PRC)
        {
            pid = change.get("0,0");
            if (pid == "0,0")
            {
                pid = ele.random_get();
                if (pid == "0,0")return;
            }
            rem.add(pid);
            
        }
        else
        {
            pid = ele.random_get();
            if (pid == "0,0")return;
            rem.add(pid);
            
        }
    }
    tree_cmd(t, pid, uid, value, ele).exec(c);
}
