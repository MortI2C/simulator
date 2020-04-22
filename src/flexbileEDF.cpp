#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "flexibleEDF.hpp"
#include "placementPolicy.hpp"
#include "minFragPolicy.hpp"
#include "qosPolicy.hpp"
using namespace std;

//Insert into vector ordered by deadline step, worst-case O(n)
void FlexibleEarliestDeadlineScheduler::insertOrderedByDeadline(vector<workload>& workloads, vector<int>& vect, int i, workload& wload) {
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

void FlexibleEarliestDeadlineScheduler::insertOrderedByAlpha(vector<workload>& workloads, vector<int>& vect, int i, workload& wload, int layoutTotalBw, int layoutTotalCapacity, int layoutFreeCores) {
    int completionTime = wload.deadline;
    bool inserted = false;
    double percFreecores = (layoutFreeCores > 0 ) ? (int)(wload.cores/layoutFreeCores)*100 : 1;
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

bool FlexibleEarliestDeadlineScheduler::scheduleWorkloads(vector<workload>& workloads,
                                     vector <int>& pendingToSchedule,
                                     vector <int>& runningWorkloads,
                                     PlacementPolicy* placementPolicy, int step, Layout& layout) {
    vector<int> orderedWorkloads;
    int layoutTotalBw = layout.getTotalBandwidth();
    int layoutTotalCapacity = layout.getTotalCapacity();
    int layoutFreeCores = layout.getFreeCores();

    for(auto it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
        workload wload = workloads[*it];
//        this->insertOrderedByAlpha(workloads,orderedWorkloads,*it,wload, layoutTotalBw, layoutTotalCapacity, layoutFreeCores );
        this->insertOrderedByDeadline(workloads,orderedWorkloads,*it,wload);
    }

    MinFragPolicy* minFrag = new MinFragPolicy(placementPolicy->model);
    QoSPolicy* qosPolicy = new QoSPolicy(placementPolicy->model);
    loadFactors currLfs = layout.calculateAbstractLoadFactors(workloads, runningWorkloads);
    if(currLfs.bandwidthLF>this->placementPolicyThreshold && currLfs.capacityLF<this->placementPolicyThreshold) {
        placementPolicy = qosPolicy;
    }
    else
        placementPolicy = minFrag;

    vector<int> toFinish;
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