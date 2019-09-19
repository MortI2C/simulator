#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "EDFStarv.hpp"
#include "placementPolicy.hpp"
using namespace std;

//Insert into vector ordered by deadline step, worst-case O(n)
void EarliestDeadlineStarvationScheduler::insertOrderedByDeadline(vector<workload>& workloads, vector<int>& vect, int i, workload& wload) {
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        int maxDelay = (wload.deadline - wload.executionTime) * (1 - this->starvCoefficient);
        if (wload.highprio) maxDelay = wload.deadline - wload.executionTime;
        if(maxDelay < workloads[*it].deadline) {
            inserted = true;
            vect.insert(it,i);
        }
    }
    if(!inserted)
        vect.push_back(i);
}

bool EarliestDeadlineStarvationScheduler::scheduleWorkloads(vector<workload>& workloads,
                                     vector <int>& pendingToSchedule,
                                     vector <int>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<int> orderedWorkloads;
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        workload wload = workloads[*it];
        this->insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
    }

    vector<int> toFinish;
    for(auto it = orderedWorkloads.begin(); it!=orderedWorkloads.end(); ++it) {
        int maxDelay = workloads[*it].deadline - workloads[*it].executionTime;
//        if(workloads[*it].highprio) maxDelay = workloads[*it].deadline;
////        int maxDelay = workloads[*it].executionTime * this->starvCoefficient + workloads[*it].arrival;
//        int deadline = (step >= maxDelay) ? -1 : workloads[*it].deadline;
        if(((step >= maxDelay) || layout.resourcesUsed() <= 0.7)
            && placementPolicy->placeWorkload(workloads,*it,layout,step,workloads[*it].deadline)) {
            workloads[*it].scheduled = step;
            runningWorkloads.push_back(*it);
//            insertOrderedByStep(workloads, runningWorkloads,*it,workloads[*it]);
            toFinish.push_back(*it);
            this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
//            cout << "workload: " << *it << " step: " << step << endl;
//            layout.printRaidsInfo();
        }
    }

    //Remove already placed workloads
    for(auto it = toFinish.begin(); it!=toFinish.end(); ++it) {
        for(auto it2 = pendingToSchedule.begin(); it2!=pendingToSchedule.end(); ++it2) {
            if(*it2 == *it) {
                pendingToSchedule.erase(it2);
                break;
            }
        }
    }
}