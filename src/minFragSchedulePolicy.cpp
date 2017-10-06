#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "minFragSchedulePolicy.hpp"
#include "placementPolicy.hpp"
using namespace std;

struct workloadFrag {
    double fragmentation;
    vector<workload>::iterator wload;
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

bool MinFragScheduler::scheduleWorkloads(vector <workload>& pendingToSchedule,
                                     vector <workload>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<vector<workload>::iterator> toFinish;
    vector<workloadFrag> potentialSchedule;
    //Recurse workloads pending and order possible to schedule workloads by fragmentation level
    for(vector<workload>::iterator it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
//        cout << "going to compose?" << endl;
        if(placementPolicy->placeWorkload(it,layout)) {
//            cout << "composing: " << it->nvmeBandwidth << " " << it->nvmeCapacity << endl;
            workloadFrag element = {layout.calculateFragmentation(),it};
            insertOrderedByFrag(potentialSchedule,element);
            placementPolicy->freeResources(*it);
        }
    }

    //Try to schedule potential schedule workloads
    for(vector<workloadFrag>::iterator it = potentialSchedule.begin(); it!=potentialSchedule.end(); ++it) {
        if(placementPolicy->placeWorkload(it->wload,layout)) {
            it->wload->scheduled = step;
            insertOrderedByStep(runningWorkloads,*it->wload);
            toFinish.push_back(it->wload);
        }
    }

    //Remove already placed workloads
    for(int i = 0; i<toFinish.size(); ++i) {
        pendingToSchedule.erase(toFinish[i]);
    }
}

//Insert into vector ordered by completion step, worst-case O(n)
void MinFragScheduler::insertOrderedByStep(vector<workload>& vector, workload& wload) {
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