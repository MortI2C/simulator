#ifndef ARRIVALUNIFORM_MODEL_H
#define ARRIVALUNIFORM_MODEL_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class ArrivalUniformModel {
   public:
    ArrivalUniformModel() {
    }
    void generate_arrivals(vector<workload>&, int, double);

};

#endif
