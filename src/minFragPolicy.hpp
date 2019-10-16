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
    MinFragPolicy(DegradationModel model) : PlacementPolicy(model) {}
    void insertSorted(vector<nvmeFitness>&, nvmeFitness&);
    void insertRackSorted(vector<rackFitness>&, rackFitness&);
    bool placeWorkload(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadInComposition(vector<workload>&, int, Layout&, int, int);
    bool placeExecOnlyWorkload(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadNewComposition(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadsNewComposition(vector<workload>&, vector<int>&, Layout&, int);
};

#endif
