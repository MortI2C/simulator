#ifndef MIN_FRAG_SCHEDULING_POLICY_H
#define MIN_FRAG_SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
using namespace std;

class MinFragScheduler : public SchedulingPolicy {
   public:
      bool scheduleWorkloads(vector<workload>&, vector<workload>&, PlacementPolicy*, int, Layout&);
      void insertOrderedByStep(vector<workload>& vector, workload&);
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
