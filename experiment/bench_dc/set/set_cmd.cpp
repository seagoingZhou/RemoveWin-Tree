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
        default:
            set0 = ele.randomSetGet();
            sprintf(tmp, "smembers %s",  set0.c_str());
        
    }

    auto r = static_cast<redisReply *>(redisCommand(c, tmp));
    printf("executing %s\n", tmp);
    if (r == nullptr) {
        printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, tmp);
        exit(-1);
    }
    
    switch (t) {
        case ADD:
            if (r->type == REDIS_REPLY_STRING && strcmp(r->str,"OK") == 0) {
                ele.sadd(set0, key);
            }
            break;
        case REM:
            if (r->type == REDIS_REPLY_STRING && strcmp(r->str,"OK") == 0) {
                ele.srem(set0, key);
            }
            break;
        case DIFF:
            if (r->type == REDIS_REPLY_STRING && strcmp(r->str,"OK") == 0) {
                ele.sdiff(set0, set1);
            }
                
            break;
        case INTER:
            if (r->type == REDIS_REPLY_STRING && strcmp(r->str,"OK") == 0) {
                ele.sinter(set0, set1);
            }
                
            break;
        case UNION:
            if (r->type == REDIS_REPLY_STRING && strcmp(r->str,"OK") == 0) {
                ele.sunion(set0, set1);
            }
            break;
        case MEMBERS:
            if (r->type == REDIS_REPLY_ARRAY) {
                ele.smembers(set0, r);
            }
            break;
    }
    freeReplyObject(r);
    return 0;
}

/*
switch (t) {
    case ADD:
        
        ele.sadd(set0, key);
        
        break;
    case REM:
        
        ele.srem(set0, key);
        
        break;
    case DIFF:
        
        ele.sdiff(set0, set1);
        
            
        break;
    case INTER:
        
        ele.sinter(set0, set1);
        
            
        break;
    case UNION:
        
        ele.sunion(set0, set1);
        
        break;
    case MEMBERS:
    {
        ele.smembers(set0, r);
        break;
    }
}

    */