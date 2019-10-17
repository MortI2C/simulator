#ifndef EARLIEST_STARV_SET_SCHEDULING_POLICY_H
#define EARLIEST_STARV_SET_SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
using namespace std;

class EarliestDeadlineStarvationSetsScheduler : public SchedulingPolicy {
   public:
      int starvCoefficient;
    EarliestDeadlineStarvationSetsScheduler(int starvCoefficient = 4) {
          this->starvCoefficient = starvCoefficient;
      };
      void insertOrderedByDeadline(vector<workload>&, vector<int>&, int, workload&);
      bool scheduleWorkloads(vector<workload>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&);
      void placeEDFSetStarvWorkloads(vector<workload>&, vector<int>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&);
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
