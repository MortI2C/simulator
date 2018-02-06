#ifndef PLACEMENT_POLICY_H
#define PLACEMENT_POLICY_H
#include <iostream>
#include <vector>
#include <algorithm>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
#include "degradationModel.hpp"
using namespace std;

class PlacementPolicy {
   public:
    DegradationModel model;
    double loadFactor;

    PlacementPolicy(DegradationModel);
    virtual bool placeWorkload(vector<workload>&, int, Layout&, int, int) =0;
    void freeResources(vector<workload>&, int);
    vector<int> MinSetHeuristic(vector<NvmeResource>&, vector<int>, int, int);
    void insertSortedBandwidth(vector<NvmeResource>&, vector<int>&, int);
    void insertSortedCapacity(vector<NvmeResource>&, vector<int>&, int);
    void updateRackWorkloads(vector <workload>&, int, Rack*, raid&, int);
    void updateRackWorkloadsTime(vector<workload>&, raid&);
};

#endif
