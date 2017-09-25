#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>
#include "util.cpp"
#include "math.h"
#include "workloadPoissonGenerator.hpp"
using namespace std;

vector<workload> WorkloadPoissonGenerator::generateWorkloads(int npatients, float lambdaExeTimes, float lambdaBw, float lambdaCapacity) {
    vector <workload> workloads(npatients);
    std::random_device rand_dev;
    std::mt19937 generator(rand_dev());
    for(int i = 0; i<npatients; ++i) {
        workloads[i].executionTime = (int)nextTime(1/lambdaExeTimes);
        workloads[i].nvmeBandwidth = (int)nextTime(1/lambdaBw);
        workloads[i].nvmeCapacity = (int)nextTime(1/lambdaCapacity);
    }
//
//    for(vector<workload>::iterator it = workloads.begin(); it!=workloads.end(); ++it) {
//        cout << " " << it->executionTime << "|" << it->nvmeBandwidth << "|" << it->nvmeCapacity;
//    }
//
//    cout << endl;
    return workloads;
}
