#include "set_cmd.h"
#include <string.h>


int set_cmd::exec(redisContext *c)
{
    //char name[128];
    //sprintf(name, "%ss%d", rpq_cmd_prefix[zt], OP_PER_SEC);
    char tmp[256];
    switch (t)
    {
        case ADD:
            sprintf(tmp, "sadd %s %s", set0.c_str(), key.c_str());
            break;
        case REM:
            sprintf(tmp, "srem %s %s", set0.c_str(), key.c_str());
            break;
        case DIFF:
            sprintf(tmp, "sdiffstore %s %s", set0.c_str(), set1.c_str());
            break;
        case INTER:
            sprintf(tmp, "sinterstore %s %s", set0.c_str(), set1.c_str());
            break;
        case UNION:
            sprintf(tmp, "sunionstore %s %s", set0.c_str(), set1.c_str());
            break;
        default:
            sprintf(tmp, "smembers %s",  set0.c_str());
        
    }
    auto r = static_cast<redisReply *>(redisCommand(c, tmp));

    if (r == nullptr)
    {
        printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, tmp);
        exit(-1);
    }

    
    switch (t)
    {
        case ADD:
            if (!strcmp(r->str,"OK")) {
                ele.sadd(set0, key);
            }
            break;
        case REM:
            if (!strcmp(r->str,"OK")) {
                ele.srem(set0, key);
            }
            break;
        case DIFF:
            if (!strcmp(r->str,"OK")) {
                ele.sdiff(set0, set1);
            }
                
            break;
        case INTER:
            if (!strcmp(r->str,"OK")) {
                ele.sinter(set0, set1);
            }
                
            break;
        case UNION:
            if (!strcmp(r->str,"OK")) {
                ele.sunion(set0, set1);
            }
            break;
        case MEMBERS:
        {
            ele.smembers(set0,r);
            break;
        }
    }
    freeReplyObject(r);
    return 0;
}
