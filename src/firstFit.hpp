#ifndef FIRST_FIT_POLICY_H
#define FIRST_FIT_POLICY_H
#include <iostream>
#include <vector>
#include "placementPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class FirstFitPolicy : public PlacementPolicy {
   public:
    FirstFitPolicy(DegradationModel model) : PlacementPolicy(model) { }
    void insertSorted(vector<nvmeFitness>&, nvmeFitness&);
    void insertRackSorted(vector<rackFitness>&, rackFitness&);
    bool placeWorkload(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadInComposition(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadNewComposition(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadsNewComposition(vector<workload>&, vector<int>&, Layout&, int);
};

#endif
