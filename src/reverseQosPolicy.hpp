#ifndef REVERSEQOS_POLICY_H
#define REVERSEQOS_POLICY_H
#include <iostream>
#include <vector>
#include "placementPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class ReverseQoSPolicy : public PlacementPolicy {
   public:
    ReverseQoSPolicy(DegradationModel model) : PlacementPolicy(model) { }
    void insertSorted(vector<nvmeFitness>&, nvmeFitness&);
    void insertRackSorted(vector<rackFitness>&, rackFitness&);
    bool placeWorkload(vector<workload>&, int, Layout&, int, int);
};

#endif
