#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <typeinfo>
#include "resources_structures.hpp"
#include "policy.hpp"
#include "bestFitPolicy.hpp"
#include "arrivalUniformModel.hpp"
#include "arrivalPoissonModel.hpp"
#include "workloadPoissonGenerator.hpp"
#include "layout.hpp"
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

void freeResources(workload& wload) {
    NvmeResource composedNvme = wload.allocation.allocatedRack->compositions
        [wload.allocation.composition].composedNvme;
    composedNvme.setAvailableBandwidth(composedNvme.getAvailableBandwidth()-wload.nvmeBandwidth);
    composedNvme.setAvailableCapacity(composedNvme.getAvailableCapacity()-wload.nvmeCapacity);

    if((--wload.allocation.workloadsUsing)==0) {
        wload.allocation.allocatedRack->freeComposition(wload.allocation.allocatedRack, wload.allocation.composition);
        wload.allocation = {};
    }
}

void calculateMissedDeadlines(Policy* policy, vector<workload>& workloads, int patients, Layout& layout) {
    int step = 0;
    int processedPatients = 0;
    vector<workload> runningWorkloads;
    vector<workload> pendingToSchedule;

    vector<workload>::iterator wlpointer = workloads.begin();
    while(processedPatients < patients) {
        //1st Check Workloads running finishing in this step
        bool finish = false;
        vector<vector<workload>::iterator> toFinish;
        for(vector<workload>::iterator run = runningWorkloads.begin(); !finish && run!=runningWorkloads.end(); ++run) {
            if((run->executionTime+run->scheduled)<=step) {
                cout << "freeing" << endl;
                toFinish.push_back(run);
                freeResources(*run);
            } else
                finish = true;
        }
        for(int i = 0; i<toFinish.size(); ++i) {
            runningWorkloads.erase(toFinish[i]);
        }

        //2nd schedule pending to schedule workloads
        toFinish.clear();
        for(vector<workload>::iterator it = pendingToSchedule.begin(); it!=pendingToSchedule.end(); ++it) {
            if(policy->scheduleWorkload(it,step,layout)) {
                insertOrderedByStep(runningWorkloads,*it);
                toFinish.push_back(it);
                ++processedPatients;
            }
        }
        for(int i = 0; i<toFinish.size(); ++i) {
            pendingToSchedule.erase(toFinish[i]);
        }
        //3rd try to schedule new arriving jobs
        bool stop = false;
        vector<vector<workload>::iterator> toRemove;
        while(wlpointer!=workloads.end() && processedPatients < patients && wlpointer->arrival <= step) {
            if(policy->scheduleWorkload(wlpointer,step,layout)) {
                insertOrderedByStep(runningWorkloads,*wlpointer);
                toRemove.push_back(wlpointer);
                ++processedPatients;
            } else {
                insertOrderedByStep(pendingToSchedule,*wlpointer);
                toRemove.push_back(wlpointer);
            }
            wlpointer++;
        }

        for(vector<vector<workload>::iterator>::iterator it = toRemove.begin();
                it!=toRemove.end(); ++it) {
            workloads.erase(*it);
        }
        wlpointer = workloads.begin();
        //Advance simulation step
        step++;
    }

    int missedDeadline = 0;
    int waitingTime = 0;
    int exeTime = 0;
    for(vector<workload>::iterator it = workloads.begin(); it != workloads.end(); it++) {
        if((it->executionTime+it->scheduled) > step) step = (it->executionTime+it->scheduled);
        if(it->deadline < (it->executionTime+it->scheduled)) ++missedDeadline;
        waitingTime += (it->scheduled - it->arrival);
        exeTime += ((it->executionTime+it->scheduled) - it->scheduled);
    }
    waitingTime/=patients;
    exeTime/=patients;

    double ratio = (double)missedDeadline/patients;
    ratio*=100;
    cout << step << " " << missedDeadline << " " << waitingTime << " " << exeTime << endl;
}

int main(int argc, char* argv[]) {
    int patients=atoi(argv[1]);
    double prio_threshold = 0.2;
    if(argc>2)
        prio_threshold=atof(argv[2]);

    WorkloadPoissonGenerator* generator = new WorkloadPoissonGenerator();
    vector<workload> workloads = generator->generateWorkloads(patients, 1499*1.5, 2000, 1000);
    ArrivalPoissonModel* arrival = new ArrivalPoissonModel();
    arrival->generate_arrivals(workloads, 99*patients, prio_threshold);

    Layout layout = Layout();
    layout.generateLayout("layouts/layout-1.json");
    BestFitPolicy* bestFit = new BestFitPolicy();
    calculateMissedDeadlines(bestFit, workloads, patients, layout);

    return 0;
}
