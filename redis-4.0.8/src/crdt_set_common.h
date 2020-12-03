#include "server.h"

void sunionResult(client *c, robj **setkeys, int setnum, robj *dstset);
void sinterResult(client *c, int idx, int setnum, robj *dstset);
void sdiffResult(client *c, int idx, int setnum, robj *dstset);

setTypeIterator *setTypeInitSafeIterator(robj *subject);