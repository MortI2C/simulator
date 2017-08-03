#ifndef ARRIVALUNIFORM_MODEL_H
#define ARRIVALUNIFORM_MODEL_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class ArrivalUniformModel {
   public:
    ArrivalUniformModel(int baseExecutionTime = 1499) {
        this->baseExecutionTime = baseExecutionTime;
    }
    vector<workload> generate_arrivals(int, double, int);
   private:
    int baseExecutionTime;
};

#endif
