#ifndef FCFS_SCHEDULING_POLICY_H
#define FCFS_SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
using namespace std;

class FcfsScheduler : public SchedulingPolicy {
   public:
      bool scheduleWorkloads(vector<workload>&, vector<workload>&, PlacementPolicy*, int, Layout&);
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
