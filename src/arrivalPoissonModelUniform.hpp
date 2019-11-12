#ifndef ARRIVALPOISSON_MODEL_UNIFORM_H
#define ARRIVALPOISSON_MODEL_UNIFORM_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class ArrivalPoissonModelUniform {
   public:
    ArrivalPoissonModelUniform() {
    }
    void generate_arrivals(vector<workload>&, float, double, double);
};

#endif
