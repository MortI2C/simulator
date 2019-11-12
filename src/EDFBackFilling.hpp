#ifndef EARLIESTSBF_SCHEDULING_POLICY_H
#define EARLIESTBF_SCHEDULING_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
using namespace std;

class EarliestDeadlineBackfillingScheduler : public SchedulingPolicy {
    public:
        EarliestDeadlineBackfillingScheduler() {};
        void insertOrderedByDeadline(vector<workload>&, vector<int>&, int, workload&);
        bool scheduleWorkloads(vector<workload>&, vector<int>&, vector<int>&, PlacementPolicy*, int, Layout&);
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
