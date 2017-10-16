#ifndef BEST_FIT_POLICY_H
#define BEST_FIT_POLICY_H
#include <iostream>
#include <vector>
#include "placementPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class BestFitPolicy : public PlacementPolicy {
   public:
    void insertSorted(vector<nvmeFitness>&, nvmeFitness);
    bool placeWorkload(vector<workload>&, int, Layout&, int);
};

#endif
