#ifndef EARLIEST_SCHEDULING_POLICY_H
#define EARLIEST_SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
using namespace std;

class EarliestDeadlineScheduler : public SchedulingPolicy {
   public:
      int starvCoefficient;
      EarliestDeadlineScheduler(int starvCoefficient = 4) {
          this->starvCoefficient = starvCoefficient;
      };
      void insertOrderedByDeadline(vector<workload>&, vector<int>&, int, workload&);
      void insertOrderedByAlpha(vector<workload>&, vector<int>&, int, workload&, int, int, int);
      bool scheduleWorkloads(vector<workload>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&);
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
