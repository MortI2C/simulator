#ifndef RANDOM_SCHEDULING_POLICY_H
#define RANDOM_SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
using namespace std;

class RandomScheduler : public SchedulingPolicy {
   public:
      bool scheduleWorkloads(vector<workload>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&);
};

#endif
