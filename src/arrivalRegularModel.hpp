#ifndef ARRIVALREGULAR_MODEL_H
#define ARRIVALREGULAR_MODEL_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class ArrivalRegularModel {
   public:
    ArrivalRegularModel() {
    }
    void generate_arrivals(vector<workload>&, int, double);

};

#endif
