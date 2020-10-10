#include "util.h"

#include <thread>
#include <random>
#include <chrono>
#include <mutex>

using namespace std;

int intRand(int min, int max){

    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_real_distribution<int> distribution(min,max);
    return distribution(*rand_gen);
}

int intRand(int max){
    return intRand(0,max);
}

double doubleRand(double min, double max){
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_real_distribution<double> distribution(min,max);
    return distribution(*rand_gen);

}

double decide(){
    return doubleRand(0.0,1.0);
}