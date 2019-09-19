#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "EDFSetStarv.hpp"
#include "placementPolicy.hpp"
using namespace std;

//Insert into vector ordered by deadline step, worst-case O(n)
void EarliestDeadlineStarvationSetsScheduler::insertOrderedByDeadline(vector<workload>& workloads, vector<int>& vect, int i, workload& wload) {
    int completionTime = wload.deadline;
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        int maxDelay = wload.deadline - wload.executionTime;
//        int maxDelay = wload.deadline * (1 - this->starvCoefficient);
//        if (wload.highprio) maxDelay = wload.deadline;
        if(maxDelay < (workloads[*it].deadline - workloads[*it].executionTime)) {
            inserted = true;
            vect.insert(it,i);
        }
    }
    if(!inserted)
        vect.push_back(i);
}

bool EarliestDeadlineStarvationSetsScheduler::scheduleWorkloads(vector<workload>& workloads,
                                     vector <int>& pendingToSchedule,
                                     vector <int>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<int> orderedWorkloads;
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        workload wload = workloads[*it];
        insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
    }

    int prior = pendingToSchedule.size();
    vector<int> toFinish;
    bool placed = false;
    int groupsSize = 5;
    for(int i = groupsSize; !placed && i<orderedWorkloads.size() && layout.resourcesUsed() <= 0.8; ++i) {
        vector<int> wset(i);
        std::copy(orderedWorkloads.begin()+(i-groupsSize), orderedWorkloads.begin() + i+1, wset.begin());
        workload wload = workloads[*(wset.begin())];
        int delay = wload.deadline - wload.executionTime;
        if ( placementPolicy->placeWorkloadsNewComposition(workloads, wset, layout, step)) {
            for (auto it = wset.begin(); it != wset.end(); ++it) {
                workloads[*it].scheduled = step;
                runningWorkloads.push_back(*it);
                toFinish.push_back(*it);
                this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
//                cout << "big compo workload: " << *it << " step: " << step << endl;
//                layout.printRaidsInfo();
            }
            placed = true;
        }
    }
    //Remove placed workloads
    for(auto it = toFinish.begin(); it!=toFinish.end(); ++it) {
        for(auto it2 = pendingToSchedule.begin(); it2!=pendingToSchedule.end(); ++it2) {
            if(*it2 == *it) {
                pendingToSchedule.erase(it2);
                break;
            }
        }
    }
    toFinish.clear();
    orderedWorkloads.clear();
    //Finally try to place sets of workloads in new compositions
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        workload wload = workloads[*it];
        insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
    }

    //Second place workloads starved for too long
    for(auto it = orderedWorkloads.begin(); it!=orderedWorkloads.end(); ++it) {
//        int maxDelay = workloads[*it].executionTime*this->starvCoefficient + workloads[*it].arrival;
        int deadline = (step > deadline) ? -1 : workloads[*it].deadline;
        int maxDelay = workloads[*it].deadline - workloads[*it].executionTime;
        if(((step >= maxDelay) || layout.resourcesUsed() <= 0.7) &&
          placementPolicy->placeWorkload(workloads,*it,layout,step,deadline)) {
            workloads[*it].scheduled = step;
            runningWorkloads.push_back(*it);
//            insertOrderedByStep(workloads, runningWorkloads,*it,workloads[*it]);
            toFinish.push_back(*it);
            this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
//            cout << "workload: " << *it << " step: " << step << endl;
//            layout.printRaidsInfo();
        }
    }
//
//    //First try to place workloads in existing compositions
//    for(auto it = orderedWorkloads.begin(); it!=orderedWorkloads.end(); ++it) {
//        int maxDelay = workloads[*it].executionTime*this->starvCoefficient + workloads[*it].arrival;
//        int deadline = (step > maxDelay) ? -1 : workloads[*it].deadline;
//        if(step <= maxDelay
//           && placementPolicy->placeWorkloadInComposition(workloads,*it,layout,step,workloads[*it].deadline)) {
//            workloads[*it].scheduled = step;
//            runningWorkloads.push_back(*it);
////            insertOrderedByStep(workloads, runningWorkloads,*it,workloads[*it]);
//            toFinish.push_back(*it);
//            this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
////            cout << "workload: " << *it << " step: " << step << endl;
////            layout.printRaidsInfo();
//        }
//    }


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