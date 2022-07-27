#ifndef MIN_FRAG_RESAWARE_POLICY_H
#define MIN_FRAG_RESAWARE_POLICY_H
#include <iostream>
#include <vector>
#include "placementPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class MinFragResAwarePolicy : public PlacementPolicy {
   public:
    MinFragPolicy(DegradationModel model) : PlacementPolicy(model) {}
    void insertSorted(vector<nvmeFitness>&, nvmeFitness&);
    void insertRackSorted(vector<rackFitness>&, rackFitness&);
    void insertRackSorted2(vector<rackFitness>&, rackFitness&);
    void insertRackSortedGpu(vector<rackFitness>&, rackFitness&);
    bool placeWorkload(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadInComposition(vector<workload>&, int, Layout&, int, int);
    bool placeExecOnlyWorkload(vector<workload>&, int, Layout&, int, int);
    bool placeGpuOnlyWorkload(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadNewComposition(vector<workload>&, int, Layout&, int, int);
    bool placeWorkloadsNewComposition(vector<workload>&, vector<int>&, Layout&, int);
    Rack* allocateCoresOnly(vector<workload>&, int, Layout&);
    Rack* allocateWorkloadsCoresOnly(vector<workload>&, vector<int>&, Layout&);
};

#endif