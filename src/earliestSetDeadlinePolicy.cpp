#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "earliestSetDeadlinePolicy.hpp"
#include "placementPolicy.hpp"
using namespace std;

//Insert into vector ordered by deadline step, worst-case O(n)
void EarliestSetDeadlineScheduler::insertOrderedByDeadline(vector<workload>& workloads, vector<int>& vect, int i, workload& wload) {
    int completionTime = wload.deadline;
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if(completionTime > workloads[*it].deadline) {
            inserted = true;
            vect.insert(it,i);
        }
    }
    if(!inserted)
        vect.push_back(i);
}

bool EarliestSetDeadlineScheduler::scheduleWorkloads(vector<workload>& workloads,
                                     vector <int>& pendingToSchedule,
                                     vector <int>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<int> orderedWorkloads;
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        workload wload = workloads[*it];
        insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
    }

    vector<int> toFinish;

    //First try to place workloads in existing compositions
    for(auto it = orderedWorkloads.begin(); it!=orderedWorkloads.end(); ++it) {
        int maxDelay = workloads[*it].executionTime*this->starvCoefficient + workloads[*it].arrival;
        if(step <= maxDelay
           && placementPolicy->placeWorkloadInComposition(workloads,*it,layout,step,workloads[*it].deadline)) {
            workloads[*it].scheduled = step;
            runningWorkloads.push_back(*it);
//            insertOrderedByStep(workloads, runningWorkloads,*it,workloads[*it]);
            toFinish.push_back(*it);
            this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
//            cout << step << endl;
//            layout.printRaidsInfo();
        }
    }
    //Second place workloads starved for too long
    for(auto it = orderedWorkloads.begin(); it!=orderedWorkloads.end(); ++it) {
        int maxDelay = workloads[*it].executionTime*this->starvCoefficient + workloads[*it].arrival;
        if(step > maxDelay && placementPolicy->placeWorkload(workloads,*it,layout,step,-1)) {
            workloads[*it].scheduled = step;
            runningWorkloads.push_back(*it);
//            insertOrderedByStep(workloads, runningWorkloads,*it,workloads[*it]);
            toFinish.push_back(*it);
            this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
//            cout << step << endl;
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

    //Finally try to place sets of workloads in new compositions
    toFinish.clear();
    orderedWorkloads.clear();
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        workload wload = workloads[*it];
        insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
    }
//
//    for(auto it = orderedWorkloads.begin(); it!=orderedWorkloads.end(); ++it) {
//        cout << *it << " ";
//    }
//    if(orderedWorkloads.size()>0)
//    cout << endl;

    bool placed = false;
    for(int i = orderedWorkloads.size(); !placed && i>0; --i) {
        vector<int> wset(i);
        std::copy(orderedWorkloads.begin(), orderedWorkloads.begin() + i, wset.begin());
        if (placementPolicy->placeWorkloadsNewComposition(workloads, wset, layout, step)) {
            for (auto it = wset.begin(); it != wset.end(); ++it) {
                workloads[*it].scheduled = step;
                runningWorkloads.push_back(*it);
                toFinish.push_back(*it);
                this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
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
}