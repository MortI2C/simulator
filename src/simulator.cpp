#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <typeinfo>
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
#include "placementPolicy.hpp"
//#include "bestFitPolicy.hpp"
#include "minFragPolicy.hpp"
#include "qosPolicy.hpp"
#include "fcfsSchedulePolicy.hpp"
#include "minFragSchedulePolicy.hpp"
#include "arrivalUniformModel.hpp"
#include "arrivalPoissonModel.hpp"
#include "arrivalRegularModel.hpp"
#include "workloadPoissonGenerator.hpp"
#include "earliestDeadlinePolicy.hpp"
#include "layout.hpp"
using namespace std;

const int BASE_TIME = 1587;

double getAvgCompletionTime(int step, const vector<workload>& scheduledWorkloads) {
    double completionTime = 0;
    for(auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it) {
        completionTime += (it->stepFinished-it->arrival);
    }
    completionTime/=scheduledWorkloads.size();

    return completionTime;
}

double getAvgExeTime(int step, const vector<workload>& scheduledWorkloads) {
    double exeTime = 0;
    for(auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it) {
        exeTime += (it->stepFinished-it->scheduled);
    }
    exeTime/=scheduledWorkloads.size();

    return exeTime;
}

double getAvgWaitingTime(int step, const vector<workload>& scheduledWorkloads) {
    double waitingTime = 0;
    for(auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it) {
        waitingTime += (it->scheduled - it->arrival);
    }
    waitingTime/=scheduledWorkloads.size();

    return waitingTime;
}

void printStatistics(int step, const vector<workload>& scheduledWorkloads) {
    int waitingTime = 0;
    int exeTime = 0;
    int completionTime = 0;
    int missedDeadlines = 0;
    for(auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it) {
        waitingTime += (it->scheduled - it->arrival);
        exeTime += (it->stepFinished-it->scheduled);
        completionTime += (it->stepFinished-it->arrival);
        if(it->stepFinished > it->deadline)
            ++missedDeadlines;
    }
    waitingTime/=scheduledWorkloads.size();
    exeTime/=scheduledWorkloads.size();
    completionTime/=scheduledWorkloads.size();

    cout << step << " " << waitingTime << " " << exeTime << " " << completionTime << " " << missedDeadlines << endl;
}

void simulator(SchedulingPolicy* scheduler, PlacementPolicy* placementPolicy, vector<workload>& workloads, int patients, Layout& layout) {
    std::cout.unsetf(std::ios::floatfield);
    std::cout.precision(4);
    int step = 0;
    int processedPatients = 0;
//    int raidsUsed = 0;
    double frag = 0;
    double resourcesUsed = 0;
    double loadFactor = 0;
    double actualLoadFactor = 0;

    vector<int> runningWorkloads;
    vector<int> pendingToSchedule;
//    vector<workload> scheduledWorkloads(patients);
    vector<workload>::iterator wlpointer = workloads.begin();
    while(processedPatients < patients || !runningWorkloads.empty()) {
        //1st Check Workloads running finishing in this step
        vector<int> toFinish;
        for(auto it = runningWorkloads.begin(); it!=runningWorkloads.end(); ++it) {
            workload* run = &workloads[*it];
            run->timeLeft--;
//            if((run->executionTime+run->scheduled)<=step) {
            if(run->timeLeft<=0) {
                workloads[*it].stepFinished = step;
                toFinish.push_back(*it);
//                scheduledWorkloads.push_back(*run);/
//                cout << "free " << workloads[*it].arrival << " step " << step << " total exe time: " << step-workloads[*it].arrival << endl;
                placementPolicy->freeResources(workloads,*it);
            }
        }

        //Remove already placed workloads
        for(auto it = toFinish.begin(); it!=toFinish.end(); ++it) {
            for(auto it2 = runningWorkloads.begin(); it2!=runningWorkloads.end(); ++it2) {
                if(*it2 == *it) {
                    runningWorkloads.erase(it2);
                    break;
                }
            }
        }

        //2nd add arriving workloads to pending to schedule
        while(wlpointer != workloads.end() && wlpointer->arrival <= step) {
            pendingToSchedule.push_back(wlpointer->wlId);
            ++wlpointer;
        }

        //3rd schedule new workloads
        //Postcondition: workloads to run inserted in vector in completion step order!
        int priorScheduler = pendingToSchedule.size();
        scheduler->scheduleWorkloads(workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
        processedPatients += (priorScheduler - pendingToSchedule.size());

        double currResourcesUsed = layout.resourcesUsed();
        double currFrag = layout.calculateFragmentation();
        double currLoadFactor = layout.loadFactor(workloads, pendingToSchedule,runningWorkloads);
        double currActLoadFactor = layout.actualLoadFactor(workloads,runningWorkloads);
        frag+=currFrag;
        resourcesUsed+=currResourcesUsed;
        loadFactor+=currLoadFactor;
        actualLoadFactor+=currActLoadFactor;

        placementPolicy->loadFactor = currLoadFactor;
        int raids = layout.raidsUsed();
        double size = layout.avgRaidSize();
//        cout << step << " " << currLoadFactor << " " << currActLoadFactor << " " << currResourcesUsed << " " << pendingToSchedule.size() << " " << currFrag << endl;
//        cout << step << " " << layout.loadFactor(workloads, pendingToSchedule,runningWorkloads) <<
//             " " << layout.actualLoadFactor(workloads,runningWorkloads) << endl;
//        layout.printRaidsInfo();
//        cout << step << " " << raids << " " << size << " " << size*raids << " " << layout.workloadsRaid() << endl;
//        cout << step << " " << layout.resourcesUsed() << " " << layout.calculateFragmentation() << endl;
//        cout << step << " " << layout.calculateFragmentation() << endl;
        ++step;
    }
    step--; //correction

//    cout << patients << " " << step << " " << loadFactor/step << " " << actualLoadFactor/step << " " << resourcesUsed/step << " " << getAvgExeTime(step, workloads) << " " << getAvgWaitingTime(step, workloads) << " " << frag/step << endl;
//    cout << loadFactor/step << " " << step << " " << actualLoadFactor/step << " " << resourcesUsed/step << " " << frag/step << endl;
//    cout << step << " " << frag/step << " " << resourcesUsed/step << endl;

//    cout << loadFactor/step << " ";
    printStatistics(step, workloads);
}

int main(int argc, char* argv[]) {
    int patients=atoi(argv[1]);
    double prio_threshold = 0.2;
//    if(argc>2)
//        prio_threshold=atof(argv[2]);

    string layoutPath = "layouts/layout-1.json";
    if(argc>2)
       layoutPath = argv[2];

    if(argc>3)
        prio_threshold=atof(argv[3]);


    WorkloadPoissonGenerator* generator = new WorkloadPoissonGenerator();
//    vector<workload> workloads = generator->generateWorkloads(patients, 1499*1.5, 2000, 1600, 4000, 3000);
    vector<workload> workloads(patients);
    for(int i = 0; i<patients; ++i) {
        workloads[i].executionTime = 1500;
        workloads[i].nvmeBandwidth = 400;
        workloads[i].nvmeCapacity = 43;
        workloads[i].wlId = i;
    }

    ArrivalPoissonModel* arrival = new ArrivalPoissonModel();
    //cluster experiments
//    arrival->generate_arrivals(workloads, 99*patients, prio_threshold);
//    arrival->generate_arrivals(workloads, 132.29, prio_threshold);
    arrival->generate_arrivals(workloads, 2400, prio_threshold);

    Layout layout = Layout();
    layout.generateLayout(layoutPath);
//    BestFitPolicy* bestFit = new BestFitPolicy();
    MinFragPolicy* minFrag = new MinFragPolicy();
    QoSPolicy* qosPolicy = new QoSPolicy();
//    MinFragScheduler* scheduler = new MinFragScheduler();
    FcfsScheduler* fcfsSched = new FcfsScheduler();
    EarliestDeadlineScheduler* earliestSched = new EarliestDeadlineScheduler();

//    cout << "bestfit: ";
//    simulator(scheduler, bestFit, workloads, patients, layout);
//    cout << "worstfit: ";
//    simulator(scheduler, worstFit, workloads, patients, layout);
//    cout << "randomfit: ";
//    simulator(scheduler, randomFit, workloads, patients, layout);
//    cout << "minfrag: ";
    vector<workload> copyWL = workloads;
    simulator(fcfsSched, minFrag, copyWL, patients, layout);
    simulator(earliestSched, qosPolicy, copyWL, patients, layout);
//    simulator(fcfsSched, minFrag, copyWL, patients, layout);
//    cout << "worst fit: ";
//    simulator(fcfsSched, worstFit, workloads, patients, layout);
//    cout << "bestfit fcfs: ";
//    simulator(fcfsSched, bestFit, workloads, patients, layout);

    return 0;
}
