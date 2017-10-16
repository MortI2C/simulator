#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "minFragSchedulePolicy.hpp"
#include "placementPolicy.hpp"
using namespace std;

struct workloadFrag {
    double fragmentation;
    vector<int>::iterator wload;
};

void insertOrderedByFrag(vector<workloadFrag>& vector, workloadFrag element) {
    bool inserted = false;
    for(std::vector<workloadFrag>::iterator it = vector.begin(); !inserted && it!=vector.end(); ++it) {
        if(element.fragmentation <= it->fragmentation) {
            inserted = true;
            vector.insert(it,element);
        }
    }
    if(!inserted)
        vector.push_back(element);
}

bool MinFragScheduler::scheduleWorkloads(vector<workload>& workloads,
                                     vector <int>& pendingToSchedule,
                                     vector <int>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<vector<int>::iterator> toFinish;
    vector<workloadFrag> potentialSchedule;
    //Recurse workloads pending and order possible to schedule workloads by fragmentation level
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
//        cout << "going to compose?" << endl;
        if(placementPolicy->placeWorkload(workloads,*it,layout,step)) {
//            cout << "composing: " << it->nvmeBandwidth << " " << it->nvmeCapacity << endl;
            workloadFrag element = {layout.calculateFragmentation(),it};
            insertOrderedByFrag(potentialSchedule,element);
            placementPolicy->freeResources(workloads,*it);
        }
    }

    //Try to schedule potential schedule workloads
    for(vector<workloadFrag>::iterator it = potentialSchedule.begin(); it!=potentialSchedule.end(); ++it) {
        if(placementPolicy->placeWorkload(workloads,*(it->wload),layout,step)) {
            workloads[*it->wload].scheduled = step;
            insertOrderedByStep(runningWorkloads,workloads, *(it->wload));
            toFinish.push_back(it->wload);
        }
    }

    //Remove already placed workloads
    for(int i = 0; i<toFinish.size(); ++i) {
        pendingToSchedule.erase(toFinish[i]);
    }
}

//Insert into vector ordered by completion step, worst-case O(n)
void MinFragScheduler::insertOrderedByStep(vector<int>& vect, vector<workload>& workloads, int wload) {
    int completionTime = workloads[wload].executionTime+workloads[wload].scheduled;
    bool inserted = false;
    for(vector<int>::iterator it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        int currentCompletion = workloads[*it].executionTime+workloads[*it].scheduled;
        if(completionTime > currentCompletion) {
            inserted = true;

            vect.insert(it,wload);
        }
    }
    if(!inserted)
        vect.push_back(wload);
}