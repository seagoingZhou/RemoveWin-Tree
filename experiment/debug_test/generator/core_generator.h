#ifndef _CORE_GENERATOR_H_
#define _CORE_GENERATOR_H_

#include "generator.h"
#include "uniform_generator.h"
#include "zipfian_generator.h"

#include <string>

using namespace ycsbc;
using namespace std;
using std::string;

#define UNIFORM 0;
#define ZIPFIAN 1;

class CoreGenerator{

    private:
    Generator<uint64_t> *keychooser;
    int zeropading;
    int minimal;
    int maximal;

   

    public:
     CoreGenerator(int type,int zeronum,int min, int max){
        zeropading = zeronum;
        minimal = min;
        maximal = max;
        if ( type==0 ) {
            keychooser = new UniformGenerator(minimal,maximal);
        } else {
            keychooser = new ZipfianGenerator(minimal,maximal);
        }
    }
    ~CoreGenerator();

    string buildKeyName(){
        uint64_t keynum = keychooser->Next();
        string value = to_string(keynum);
        int fill = zeropading - value.size();

        string prev = "node";
        string pading(fill,'0');

        return prev+pading+value;
    }
};

#endif