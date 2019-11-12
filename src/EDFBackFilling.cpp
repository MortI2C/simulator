#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "EDFBackFilling.hpp"
#include "placementPolicy.hpp"
using namespace std;

//Insert into vector ordered by deadline step, worst-case O(n)
void EarliestDeadlineBackfillingScheduler::insertOrderedByDeadline(vector<workload>& workloads, vector<int>& vect, int i, workload& wload) {
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if(!workloads[*it].highprio && wload.highprio) {
            vect.insert(it, i);
            inserted = true;
        } else if((wload.highprio && workloads[*it].highprio) || (!wload.highprio && !workloads[*it].highprio)) {
            if (wload.deadline < workloads[*it].deadline) {
                inserted = true;
                vect.insert(it, i);
            }
        }
    }
    if(!inserted)
        vect.push_back(i);
}

bool EarliestDeadlineBackfillingScheduler::scheduleWorkloads(vector<workload>& workloads,
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
        if(placementPolicy->placeWorkload(workloads,*it,layout,step,workloads[*it].deadline)) {
            workloads[*it].scheduled = step;
            runningWorkloads.push_back(*it);
//            insertOrderedByStep(workloads, runningWorkloads,*it,workloads[*it]);
            toFinish.push_back(*it);
            this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
//            cout << "workload: " << *it << " step: " << step << endl;
//            layout.printRaidsInfo();
        } else if(workloads[*it].highprio && step < maxDelay) //RESERVE FURTHER RESOURCES FOR HIGHPRIO JOB
            break;
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