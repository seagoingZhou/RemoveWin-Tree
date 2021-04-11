#include "tree_cmd.h"
#include <string.h>


int tree_cmd::exec(redisContext *c)
{
    //char name[128];
    //sprintf(name, "%ss%d", rpq_cmd_prefix[zt], OP_PER_SEC);
    char tmp[256];
    switch (t)
    {
        case insert:
            sprintf(tmp, "treeinsertwithid t %s %s %s",value.c_str(), pid.c_str(), uid.c_str());
            break;
        case del:
            sprintf(tmp, "treedelete t %s", pid.c_str());
            break;
        case changevalue:
            sprintf(tmp, "treechangevalue t %s %s",  pid.c_str(), value.c_str());
            break;
        case treemembers:
            sprintf(tmp, "treemembers t");
            break;
        case overhead:
            sprintf(tmp, "rwftreeovhd t");
            break;
        
    }
    auto r = static_cast<redisReply *>(redisCommand(c, tmp));
    //printf("executing %s\n", tmp);
    if (r == nullptr)
    {
        printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, tmp);
        showTotalTime();
        return -1;
    }

    
    switch (t)
    {
        case insert:
            if (!strcmp(r->str,"OK"))
                ele.insert(pid,uid,value);
            break;
        case del:
            if (!strcmp(r->str,"OK"))
                ele.del(pid);
            break;
        case changevalue:
            if (!strcmp(r->str,"OK"))
                ele.changevalue(pid,value);
            break;
        case treemembers:
        {
            ele.members(r);
            //printf("host %s:%d executing %s\n", c->tcp.host, c->tcp.port, tmp);
            break;
        }
        case overhead:
            ele.overhead(r);
            break;
    }

    freeReplyObject(r);
    return 0;
}

mutex tree_cmd::mtx;
timeval tree_cmd::tStart{};
timeval tree_cmd::tEnd{};
