#ifndef MIN_FRAG_POLICY_H
#define MIN_FRAG_POLICY_H
#include <iostream>
#include <vector>
#include "placementPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class MinFragPolicy : public PlacementPolicy {
   public:
    void insertSorted(vector<nvmeFitness>&, nvmeFitness&);
    void insertRackSorted(vector<rackFitness>&, rackFitness&);
    bool placeWorkload(vector<workload>&, int, Layout&, int, int);
};

#endif
