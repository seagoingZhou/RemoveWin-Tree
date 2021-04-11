#include "set_cmd.h"
#include <string.h>


int set_cmd::exec(redisContext *c) {
    
    char tmp[256];
    switch (t) {
        case ADD:
            sprintf(tmp, "%ssadd %s %s",set_type.c_str(), set0.c_str(), key.c_str());
            break;
        case REM:
            sprintf(tmp, "%ssrem %s %s", set_type.c_str(), set0.c_str(), key.c_str());
            break;
        case DIFF:
            sprintf(tmp, "%ssdiffstore %s %s %s", set_type.c_str(), set0.c_str(), set0.c_str(), set1.c_str());
            break;
        case INTER:
            sprintf(tmp, "%ssinterstore %s %s %s", set_type.c_str(), set0.c_str(), set0.c_str(), set1.c_str());
            break;
        case UNION:
            sprintf(tmp, "%ssunionstore %s %s %s", set_type.c_str(), set0.c_str(), set0.c_str(), set1.c_str());
            break;
        case OVERHEAD:
            sprintf(tmp, "%ssetovhd set0", set_type.c_str());
            break;
        default:
            set0 = "set0";
            sprintf(tmp, "smembers %s",  set0.c_str());
        
    }

    auto r = static_cast<redisReply *>(redisCommand(c, tmp));
    auto r1 = static_cast<redisReply *>(redisCommand(c, "scard set0"));
    if (r == nullptr) {
        printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, tmp);
        exit(-1);
    }


    if (ele.getModel() == COMPLEX) {
        if (r1->integer > REMOTE_MAX_KEY_SIZE) {
            //ele.remoteMaxAdd(set0);
            ele.setTargetFlag(1);
        } else if (r1->integer < REMOTE_MIN_KEY_SIZE) {
            ele.setTargetFlag(-1);
        } else {
            ele.setTargetFlag(0);
        }
    } else {
        if (r1->integer > SIMPLE_MAX) {
            ele.setSimpleFlag(1);
        } else if (r1->integer < SIMPLE_MIN) {
            ele.setSimpleFlag(-1);
        } else {
            ele.setSimpleFlag(0);
        }
    }
    
    switch (t) {
        case ADD:
            if (strcmp(r->str,"OK") == 0) {
                ele.sadd(set0, key);
            }
            break;
        case REM:
            if (strcmp(r->str,"OK") == 0) {
                ele.srem(set0, key);
            }
            break;
        case DIFF:
            if (strcmp(r->str,"OK") == 0) {
                ele.sdiff(set0, set1);
            }
                
            break;
        case INTER:
            if (strcmp(r->str,"OK") == 0) {
                ele.sinter(set0, set1);
            }
                
            break;
        case UNION:
            if (strcmp(r->str,"OK") == 0) {
                ele.sunion(set0, set1);
            }
            break;
        case OVERHEAD:
            ele.soverhead(r);
            break;

        case MEMBERS:
            if (r->type == REDIS_REPLY_ARRAY) {
                ele.smembers(set0, r);
            }
            break;
    }
    freeReplyObject(r);
    freeReplyObject(r1);
    return 0;
}
