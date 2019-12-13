#include <vector>
#include <string>

#include "generator/generator.h"

class Core_Workload
{
private:
    int DELAY;
    int DELAY_LOW;
    int TOTAL_SERVERS;
    int TOTAL_OPS;
    int OP_PER_SEC;
public:
    Core_Workload(/* args */);
    ~Core_Workload();
};