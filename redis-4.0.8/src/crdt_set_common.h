#include "server.h"

void sunionResult(client *c, robj **setkeys, int setnum, robj *dstset);
void sinterResult(client *c, robj **setkeys, int setnum, robj *dstset);