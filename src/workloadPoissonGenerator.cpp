#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>
#include "util.cpp"
#include "math.h"
#include "workloadPoissonGenerator.hpp"
using namespace std;

vector<workload> WorkloadPoissonGenerator::generateWorkloads(int npatients, float lambdaExeTimes, float lambdaBw, float lambdaCapacity, int maxBw, int maxCapacity) {
    vector <workload> workloads(npatients);
    std::random_device rand_dev;
    std::mt19937 generator(5);
    for(int i = 0; i<npatients; ++i) {
        workloads[i].executionTime = (int)nextTime(1/lambdaExeTimes);
        int nvmeBw = (int)nextTime(1/lambdaBw);
        while(nvmeBw > maxBw)
            nvmeBw = (int)nextTime(1/lambdaBw);

        int nvmeCap = (int)nextTime(1/lambdaCapacity);
        while(nvmeCap > maxCapacity)
            nvmeCap = (int)nextTime(1/lambdaCapacity);

        workloads[i].nvmeBandwidth = nvmeBw;
        workloads[i].nvmeCapacity = nvmeCap;
    }

//    for(vector<workload>::iterator it = workloads.begin(); it!=workloads.end(); ++it) {
//        cout << " " << it->executionTime << "|" << it->nvmeBandwidth << "|" << it->nvmeCapacity;
//    }
//
//    cout << endl;
    return workloads;
}