#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <typeinfo>
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
#include "placementPolicy.hpp"
#include "randomFitPolicy.hpp"
#include "bestFitPolicy.hpp"
#include "worstFitPolicy.hpp"
#include "firstFitPolicy.hpp"
#include "minFragPolicy.hpp"
#include "fcfsSchedulePolicy.hpp"
#include "minFragSchedulePolicy.hpp"
#include "arrivalUniformModel.hpp"
#include "arrivalPoissonModel.hpp"
#include "workloadPoissonGenerator.hpp"
#include "layout.hpp"
using namespace std;

void printStatistics(int step, const vector<workload>& scheduledWorkloads) {
    int waitingTime = 0;
    int exeTime = 0;
    for(auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it) {
        waitingTime += (it->scheduled - it->arrival);
        exeTime += ((it->executionTime+it->scheduled) - it->arrival);
    }
    waitingTime/=scheduledWorkloads.size();
    exeTime/=scheduledWorkloads.size();

    cout << step << " " << waitingTime << " " << exeTime << endl;
}

void simulator(SchedulingPolicy* scheduler, PlacementPolicy* placementPolicy, vector<workload>& workloads, int patients, Layout& layout) {
    std::cout.unsetf(std::ios::floatfield);
    std::cout.precision(2);
    int step = 0;
    int processedPatients = 0;
//    int raidsUsed = 0;
    double frag = 0;
    double resourcesUsed = 0;

    vector<workload> runningWorkloads;
    vector<workload> pendingToSchedule;
    vector<workload> scheduledWorkloads(patients);
    vector<workload>::iterator wlpointer = workloads.begin();

    while(processedPatients < patients || !runningWorkloads.empty()) {
        //1st Check Workloads running finishing in this step
        bool finish = false;
        vector<vector<workload>::iterator> toFinish;
        for(vector<workload>::iterator run = runningWorkloads.begin(); !finish && run!=runningWorkloads.end(); ++run) {
            if((run->executionTime+run->scheduled)<=step) {
                toFinish.push_back(run);
                scheduledWorkloads.push_back(*run);
                placementPolicy->freeResources(*run);
            } else
                finish = true;
        }
        for(int i = 0; i<toFinish.size(); ++i) {
            runningWorkloads.erase(toFinish[i]);
        }

        //2nd add arriving workloads to pending to schedule
        while(wlpointer != workloads.end() && wlpointer->arrival <= step) {
            pendingToSchedule.push_back(*wlpointer);
            ++wlpointer;
        }
        //3rd schedule new workloads
        //Postcondition: workloads to run inserted in vector in completion step order!
        int priorScheduler = pendingToSchedule.size();
        scheduler->scheduleWorkloads(pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
        processedPatients += (priorScheduler - pendingToSchedule.size());

        frag+=layout.calculateFragmentation();
        resourcesUsed+=layout.resourcesUsed();

        int raids = layout.raidsUsed();
        double size = layout.avgRaidSize();
//        layout.printRaidsInfo();
//        cout << step << " " << raids << " " << size << " " << size*raids << " " << layout.workloadsRaid() << endl;
//        cout << step << " " << layout.resourcesUsed() << " " << layout.calculateFragmentation() << endl;
//        cout << step << " " << layout.calculateFragmentation() << endl;
        ++step;
    }

    cout << step << " " << frag/step << " " << resourcesUsed/step << endl;

//    printStatistics(step, scheduledWorkloads);
}

int main(int argc, char* argv[]) {
    int patients=atoi(argv[1]);
    double prio_threshold = 0.2;
    if(argc>2)
        prio_threshold=atof(argv[2]);

    WorkloadPoissonGenerator* generator = new WorkloadPoissonGenerator();
    vector<workload> workloads = generator->generateWorkloads(patients, 1499*1.5, 2000, 1600, 4000, 3000);
    ArrivalPoissonModel* arrival = new ArrivalPoissonModel();
    arrival->generate_arrivals(workloads, 99*patients, prio_threshold);

    Layout layout = Layout();
    layout.generateLayout("layouts/layout-1.json");
    BestFitPolicy* bestFit = new BestFitPolicy();
    WorstFitPolicy* worstFit = new WorstFitPolicy();
    RandomFitPolicy* randomFit = new RandomFitPolicy();
    FirstFitPolicy* firstFit = new FirstFitPolicy();
    MinFragPolicy* minFrag = new MinFragPolicy();
    MinFragScheduler* scheduler = new MinFragScheduler();

//    cout << "bestfit: ";
//    simulator(scheduler, bestFit, workloads, patients, layout);
//    cout << "worstfit: ";
//    simulator(scheduler, worstFit, workloads, patients, layout);
//    cout << "randomfit: ";
//    simulator(scheduler, randomFit, workloads, patients, layout);
//    cout << "firstfit: ";
//    simulator(scheduler, firstFit, workloads, patients, layout);
//    cout << "minfrag: ";
    simulator(scheduler, minFrag, workloads, patients, layout);

    return 0;
}
