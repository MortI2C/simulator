#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "fcfsSchedulePolicy.hpp"
#include "placementPolicy.hpp"
using namespace std;

//Insert into vector ordered by completion step, worst-case O(n)
void insertOrderedByStep(vector<workload>& workloads, vector<int>& vect, int i, workload& wload) {
    int completionTime = wload.executionTime+wload.scheduled;
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        int currentCompletion = workloads[*it].executionTime+workloads[*it].scheduled;
        if(completionTime > currentCompletion) {
            inserted = true;
            vect.insert(it,i);
        }
    }
    if(!inserted)
        vect.push_back(i);
}

bool FcfsScheduler::scheduleWorkloads(vector<workload>& workloads,
                                     vector <int>& pendingToSchedule,
                                     vector <int>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<int> toFinish;
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        int deadline = (step > workloads[*it].deadline) ? -1 : workloads[*it].deadline;
        if(placementPolicy->placeWorkload(workloads,*it,layout,step,deadline)) {
            workloads[*it].scheduled = step;
            runningWorkloads.push_back(*it);
            toFinish.push_back(*it);
            this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
        }
    }

//    if(toFinish.empty() && !pendingToSchedule.empty() && layout.resourcesUsed() < 1)
//        cerr << "ERROR" << " " << layout.resourcesUsed() << endl;
    //Remove already placed workloads
    for(auto it = toFinish.begin(); it!=toFinish.end(); ++it) {
        for(auto it2 = pendingToSchedule.begin(); it2!=pendingToSchedule.end(); ++it2) {
            if(*it2 == *it) {
                pendingToSchedule.erase(it2);
                break;
            }
        }
    }

    return true;
}