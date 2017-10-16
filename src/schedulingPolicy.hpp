#ifndef SCHEDULING_POLICY_H
#define SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "placementPolicy.hpp"
using namespace std;

class SchedulingPolicy {
   public:
    virtual bool scheduleWorkloads(vector<workload>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&) =0;
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
