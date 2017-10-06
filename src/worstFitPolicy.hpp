#ifndef WORST_FIT_POLICY_H
#define WORST_FIT_POLICY_H
#include <iostream>
#include <vector>
#include "placementPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class WorstFitPolicy : public PlacementPolicy {
   public:
    void insertSorted(vector<nvmeFitness>&, nvmeFitness);
    bool placeWorkload(vector<workload>::iterator, Layout&, int);
};

#endif
