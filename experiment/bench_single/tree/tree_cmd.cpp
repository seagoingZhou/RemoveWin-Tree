#include "tree_cmd.h"
#include <string.h>


void tree_cmd::exec(redis_client &c)
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
        
    }
    auto r = c.exec(tmp);

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
            ele.members(std::move(r));
            break;
        }
    }
}
