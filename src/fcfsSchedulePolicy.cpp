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
    vector<vector<int>::iterator> toFinish;
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        if(placementPolicy->placeWorkload(workloads,*it,layout,step)) {
            workloads[*it].scheduled = step;
            insertOrderedByStep(workloads, runningWorkloads,*it,workloads[*it]);
            toFinish.push_back(it);
//            cout << step << endl;
//            layout.printRaidsInfo();
        }
    }

    //Remove already placed workloads
    for(int i = 0; i<toFinish.size(); ++i) {
        pendingToSchedule.erase(toFinish[i]);
    }
}