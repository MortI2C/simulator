#ifndef RANDOM_FIT_POLICY_H
#define RANDOM_FIT_POLICY_H
#include <iostream>
#include <vector>
#include "placementPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class RandomFitPolicy : public PlacementPolicy {
   public:
    void insertSorted(vector<nvmeFitness>&, nvmeFitness);
    bool placeWorkload(vector<workload>::iterator, Layout&);
};

#endif
