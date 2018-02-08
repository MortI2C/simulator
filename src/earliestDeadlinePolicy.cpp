#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "earliestDeadlinePolicy.hpp"
#include "placementPolicy.hpp"
using namespace std;

//Insert into vector ordered by deadline step, worst-case O(n)
void insertOrderedByDeadline(vector<workload>& workloads, vector<int>& vect, int i, workload& wload) {
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

bool EarliestDeadlineScheduler::scheduleWorkloads(vector<workload>& workloads,
                                     vector <int>& pendingToSchedule,
                                     vector <int>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<int> orderedWorkloads;
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        workload wload = workloads[*it];
        insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
    }

    vector<int> toFinish;
    for(auto it = orderedWorkloads.begin(); it!=orderedWorkloads.end(); ++it) {
        int maxDelay = workloads[*it].executionTime*this->starvCoefficient + workloads[*it].arrival;
        int deadline = (step > maxDelay) ? -1 : workloads[*it].deadline;
        if(placementPolicy->placeWorkload(workloads,*it,layout,step,deadline)) {
            workloads[*it].scheduled = step;
            runningWorkloads.push_back(*it);
//            insertOrderedByStep(workloads, runningWorkloads,*it,workloads[*it]);
            toFinish.push_back(*it);
//            this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
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
}