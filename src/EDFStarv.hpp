#ifndef EARLIESTSTARV_SCHEDULING_POLICY_H
#define EARLIESTSTARV_SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
using namespace std;

class EarliestDeadlineStarvationScheduler : public SchedulingPolicy {
    private:
        int starvCoefficient;
    public:
        EarliestDeadlineStarvationScheduler(int starvCoefficient = 4) {
          this->starvCoefficient = starvCoefficient;
         };
        void insertOrderedByDeadline(vector<workload>&, vector<int>&, int, workload&);
        bool scheduleWorkloads(vector<workload>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&);
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
