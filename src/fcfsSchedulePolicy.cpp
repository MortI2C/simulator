#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "fcfsSchedulePolicy.hpp"
#include "placementPolicy.hpp"
using namespace std;

//Insert into vector ordered by completion step, worst-case O(n)
void insertOrderedByStep(vector<workload>& vector, workload& wload) {
    int completionTime = wload.executionTime+wload.scheduled;
    bool inserted = false;
    for(std::vector<workload>::iterator it = vector.begin(); !inserted && it!=vector.end(); ++it) {
        int currentCompletion = it->executionTime+it->scheduled;
        if(completionTime > currentCompletion) {
            inserted = true;
            vector.insert(it,wload);
        }
    }
    if(!inserted)
        vector.push_back(wload);
}

bool FcfsScheduler::scheduleWorkloads(vector <workload>& pendingToSchedule,
                                     vector <workload>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<vector<workload>::iterator> toFinish;
    for(vector<workload>::iterator it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        if(placementPolicy->placeWorkload(it,layout,step)) {
            it->scheduled = step;
            insertOrderedByStep(runningWorkloads,*it);
            toFinish.push_back(it);
        }
    }

    //Remove already placed workloads
    for(int i = 0; i<toFinish.size(); ++i) {
        pendingToSchedule.erase(toFinish[i]);
    }
}