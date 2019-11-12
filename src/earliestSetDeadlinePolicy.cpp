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
        if(completionTime < workloads[*it].deadline) {
            inserted = true;
            vect.insert(it,i);
        }
    }
    if(!inserted)
        vect.push_back(i);
}

void EarliestSetDeadlineScheduler::insertOrderedByAlpha(vector<workload>& workloads, vector<int>& vect, int i, workload& wload, int layoutTotalBw, int layoutTotalCapacity, int layoutFreeCores) {
    int completionTime = wload.deadline;
    bool inserted = false;
    double percFreecores = (layoutFreeCores > 0) ? (int)(wload.cores/layoutFreeCores)*100 : 1;
    int alpha = (((wload.nvmeBandwidth/layoutTotalBw)*100+(wload.nvmeCapacity/layoutTotalCapacity)*100)/percFreecores)*wload.executionTime;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if(!workloads[*it].highprio && wload.highprio) {
            vect.insert(it, i);
            inserted = true;
        } else if(!workloads[*it].highprio) {
            int alphaVect =
                    (workloads[*it].nvmeBandwidth / layoutTotalBw + workloads[*it].nvmeCapacity / layoutTotalCapacity) *
                    workloads[*it].executionTime;
            if (alphaVect > alpha) {
                inserted = true;
                vect.insert(it, i);
            } else if (alphaVect == alpha && wload.deadline < workloads[*it].deadline) {
                inserted = true;
                vect.insert(it, i);
            }
        }
    }
    if(!inserted)
        vect.push_back(i);
}

void EarliestSetDeadlineScheduler::placeEDFSetWorkloads(vector<workload>& workloads, vector<int>& pendingToSchedule,
                            vector<int>& scheduled, vector<int>& runningWorkloads,
                            PlacementPolicy* placementPolicy, int step, Layout& layout) {
    bool placed = false;
    int groupsSize = (pendingToSchedule.size() >= 5) ? 5 : pendingToSchedule.size();
    for(int i = groupsSize; !placed && i<pendingToSchedule.size(); ++i) {
        vector<int> wset(groupsSize);
        std::copy(pendingToSchedule.begin()+(i-groupsSize), pendingToSchedule.begin() + i, wset.begin());
//        std::copy(orderedWorkloads.begin(), orderedWorkloads.begin() + i, wset.begin());
        if (placementPolicy->placeWorkloadsNewComposition(workloads, wset, layout, step)) {
            for (auto it = wset.begin(); it != wset.end(); ++it) {
                workloads[*it].scheduled = step;
                runningWorkloads.push_back(*it);
                scheduled.push_back(*it);
                this->log(*it, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
//                cout << "big compo workload: " << *it << " step: " << step << endl;
//                layout.printRaidsInfo();
            }
            placed = true;
        }
    }
}

bool EarliestSetDeadlineScheduler::scheduleWorkloads(vector<workload>& workloads,
                                     vector <int>& pendingToSchedule,
                                     vector <int>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<int> orderedWorkloads;
    vector<int> orderedSmufinWLs;
    int layoutTotalBw = layout.getTotalBandwidth();
    int layoutTotalCapacity = layout.getTotalCapacity();
    int layoutFreeCores = layout.getFreeCores();
    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        workload wload = workloads[*it];
//        insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
        if(wload.wlName != "smufin")
            this->insertOrderedByAlpha(workloads,orderedWorkloads,*it,wload, layoutTotalBw, layoutTotalCapacity, layoutFreeCores );
        else
            this->insertOrderedByAlpha(workloads,orderedSmufinWLs,*it,wload, layoutTotalBw, layoutTotalCapacity, layoutFreeCores );
    }

    vector<int> toFinish;
    placeEDFSetWorkloads(workloads, orderedSmufinWLs, toFinish, runningWorkloads, placementPolicy, step, layout);
    placeEDFSetWorkloads(workloads, orderedWorkloads, toFinish, runningWorkloads, placementPolicy, step, layout);
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
//        insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
        this->insertOrderedByAlpha(workloads,orderedWorkloads,*it,wload, layoutTotalBw, layoutTotalCapacity, layoutFreeCores );
    }

    //Second place workloads starved for too long
    for(auto it = orderedWorkloads.begin(); it!=orderedWorkloads.end(); ++it) {
//        int maxDelay = workloads[*it].executionTime*this->starvCoefficient + workloads[*it].arrival;
        int deadline = (step > workloads[*it].deadline) ? -1 : workloads[*it].deadline;
        if(placementPolicy->placeWorkload(workloads,*it,layout,step,deadline)) {
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