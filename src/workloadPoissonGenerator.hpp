#ifndef WORKLOAD_POISSON_GENERATOR_H
#define WORKLOAD_POISSON_GENERATOR_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class WorkloadPoissonGenerator {
   public:
    WorkloadPoissonGenerator() {
    }
    vector<workload> generateWorkloads(int, float, float, float);
};

#endif
