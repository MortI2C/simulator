#ifndef SCHEDULING_POLICY_H
#define SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "placementPolicy.hpp"
#include "json.hpp"
using namespace std;
using json = nlohmann::json;

class SchedulingPolicy {
   public:
    json logger;
    virtual bool scheduleWorkloads(vector<workload>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&) =0;
    void log(int, vector<workload>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&);
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
