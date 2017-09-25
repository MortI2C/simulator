#ifndef ARRIVALPOISSON_MODEL_H
#define ARRIVALPOISSON_MODEL_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class ArrivalPoissonModel {
   public:
    ArrivalPoissonModel() {
    }
    void generate_arrivals(vector<workload>&, float, double);
};

#endif
