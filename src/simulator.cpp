#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <typeinfo>
#include <algorithm>
#include <string>
#include <random>
#include "nlohmann/json.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
#include "placementPolicy.hpp"
#include "minFragPolicy.hpp"
#include "qosPolicy.hpp"
#include "fcfsSchedulePolicy.hpp"
#include "minFragSchedulePolicy.hpp"
#include "arrivalUniformModel.hpp"
#include "arrivalPoissonModel.hpp"
#include "arrivalPoissonModelUniform.hpp"
#include "arrivalRegularModel.hpp"
#include "workloadPoissonGenerator.hpp"
#include "earliestDeadlinePolicy.hpp"
#include "earliestSetDeadlinePolicy.hpp"
#include "degradationModel.hpp"
#include "firstFitPlacement.hpp"
#include "layout.hpp"
#include "mmpp-2.hpp"
#include "EDFStarv.hpp"
#include "EDFSetStarv.hpp"
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

void printStatistics(int step, const vector<workload>& scheduledWorkloads, int stationaryStep = 0) {
    int waitingTime = 0;
    int exeTime = 0;
    int completionTime = 0;
    int missedDeadlines = 0;
    int workloadsInStationary = 0;
    for(auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it) {
        if(stationaryStep <= it->arrival ) {
            waitingTime += (it->scheduled - it->arrival);
            exeTime += (it->stepFinished - it->scheduled);
            completionTime += (it->stepFinished - it->arrival);
            if (it->stepFinished > it->deadline)
                ++missedDeadlines;
            workloadsInStationary++;
        }
    }
    if(workloadsInStationary > 0) {
        waitingTime /= workloadsInStationary;
        exeTime /= workloadsInStationary;
        completionTime /= workloadsInStationary;
    }
    cerr << step << " " << workloadsInStationary << " " << waitingTime << " " << exeTime << " " << completionTime << " " << missedDeadlines << endl;
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
    int stationaryStep = -1;

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
                scheduler->logger[*it]["completion"]=step;
//                scheduler->logger[workloads[*it].scheduled]["completion"].push_back(step);
                toFinish.push_back(*it);
//                scheduledWorkloads.push_back(*run);/
//                cout << "free " << workloads[*it].arrival << " step " << step << " total exe time: " << step-workloads[*it].arrival << endl;
                placementPolicy->freeResources(workloads,*it);
                scheduler->logger[*it]["completionLoadFactor"] = layout.loadFactor(workloads, pendingToSchedule,runningWorkloads);
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
//        if(stationaryStep==-1 && priorScheduler > 0 && (priorScheduler - pendingToSchedule.size()) <= 0)
//            stationaryStep = step;
        if(stationaryStep==-1 && layout.loadFactor(workloads, pendingToSchedule, runningWorkloads) >= 0.9 )
            stationaryStep = step;

        double currResourcesUsed = layout.resourcesUsed();
        double currFrag = layout.calculateFragmentation();
        double currLoadFactor = layout.loadFactor(workloads, pendingToSchedule,runningWorkloads);
        double currActLoadFactor = layout.actualLoadFactor(workloads,runningWorkloads);
        if(stationaryStep>=0) {
            frag += currFrag;
            resourcesUsed += currResourcesUsed;
            loadFactor += currLoadFactor;
            actualLoadFactor += currActLoadFactor;
        }

        placementPolicy->loadFactor = currLoadFactor;
        int raids = layout.raidsUsed();
        double size = layout.avgRaidSize();
//        cout << step << " " << currLoadFactor << endl;
//        cout << step << " " << currLoadFactor << " " << currActLoadFactor << " " << currResourcesUsed << " " << pendingToSchedule.size() << " " << currFrag << endl;
//        cout << step << " " << layout.loadFactor(workloads, pendingToSchedule,runningWorkloads) <<
//             " " << layout.actualLoadFactor(workloads,runningWorkloads) << endl;
//        layout.printRaidsInfo();
//        cout << step << " " << raids << " " << size << " " << size*raids << " " << layout.workloadsRaid() << endl;
//        cout << step << " " << layout.resourcesUsed() << " " << layout.calculateFragmentation() << endl;
//        cout << step << " " << layout.calculateFragmentation() << endl;
        ++step;
    }
    cout << scheduler->logger.dump() << endl;
    step--; //correction
//    printStatistics(step, workloads, stationaryStep);
    step = step - stationaryStep;
//    cout << patients << " " << step << " " << loadFactor/step << " " << actualLoadFactor/step << " " << resourcesUsed/step << " " << getAvgExeTime(step, workloads) << " " << getAvgWaitingTime(step, workloads) << " " << frag/step << endl;
//    cerr << stationaryStep << " " << loadFactor/step << " " << step << " " << actualLoadFactor/step << " " << resourcesUsed/step << " " << frag/step << endl;
//    cout << step << " " << frag/step << " " << resourcesUsed/step << endl;

//    cout << loadFactor/step << " " << getAvgExeTime(step,workloads) << endl;

}

int main(int argc, char* argv[]) {
    int patients=atoi(argv[1]);
    double lambdaCoefficient=1;
    double prio_threshold = 0.9;
    int starvCoefficient = 0.5;
//    if(argc>2)
//        prio_threshold=atof(argv[2]);

    string layoutPath = "layouts/layout-7.json";
    if(argc>2)
       layoutPath = argv[2];

    if(argc>3)
        prio_threshold = atof(argv[3]);

    if(argc>4)
        lambdaCoefficient = atof(argv[4]);

    if(argc>5)
        starvCoefficient = atoi(argv[5]);

    WorkloadPoissonGenerator* generator = new WorkloadPoissonGenerator();
//    vector<workload> workloads = generator->generateWorkloads(patients, 1499*1.5, 2000, 1600, 4000, 3000);
    vector<workload> workloads(patients);
//    std::random_device generate(5);
//    std::default_random_engine generate(5);
    std::mt19937 generate(5);
    uniform_real_distribution<double> distribution(0.0, 1.0);
//    for(int i = 0; i<patients; ++i) {
//        double number = distribution(generate);
//        if(number < 0.3) { //0.4
//            workloads[i].executionTime = 1500;
//            workloads[i].nvmeBandwidth = 1800;
//            workloads[i].baseBandwidth=1800;
//            workloads[i].nvmeCapacity = 43;
//            workloads[i].wlId = i;
//            workloads[i].performanceMultiplier=0.98;
//            workloads[i].limitPeakBandwidth=6000;
//            workloads[i].cores = 1;
//        } else if (number < 0.6) {
//            workloads[i].executionTime = 320;
//            workloads[i].nvmeBandwidth = 160;
//            workloads[i].nvmeCapacity = 341;
//            workloads[i].baseBandwidth = 160;
//            workloads[i].wlId = i;
//            workloads[i].performanceMultiplier = 1;
//            workloads[i].limitPeakBandwidth = 160;
//            workloads[i].cores = 1;
//        } else {
//            workloads[i].executionTime = 500;
//            workloads[i].nvmeBandwidth = 0;
//            workloads[i].nvmeCapacity = 0;
//            workloads[i].baseBandwidth = 0;
//            workloads[i].wlId = i;
//            workloads[i].performanceMultiplier = 1;
//            workloads[i].limitPeakBandwidth = 0;
//            workloads[i].cores = 4;
//        }
//    }
    uniform_int_distribution<int> executionTimes(100,1800);
    uniform_int_distribution<int> bandwidths(0,2000);
    uniform_int_distribution<int> capacity(0,1600);
    uniform_int_distribution<int> numCores(1,20);
    std::uniform_real_distribution<double> performanceMult(0.9,1.0);

    vector<workload> workloadsType(20);
    for(int i = 0; i<20; ++i) {
        workloadsType[i].executionTime = executionTimes(generate);
        workloadsType[i].nvmeBandwidth = bandwidths(generate);
        workloadsType[i].baseBandwidth= workloadsType[i].nvmeBandwidth;
        workloadsType[i].nvmeCapacity = capacity(generate);
        workloadsType[i].wlId = i;
        workloadsType[i].performanceMultiplier=performanceMult(generate);
        uniform_int_distribution<int> limitsBw(workloadsType[i].nvmeBandwidth, 10000);
        workloadsType[i].limitPeakBandwidth=limitsBw(generate);
        workloadsType[i].cores = numCores(generate);
    }
//    json workloadsDistribution;
    uniform_int_distribution<int> typeGenerator(0, 19);
    for(int i = 0; i<patients; ++i) {
        int wlType = typeGenerator(generate);
        workloads[i].executionTime = workloadsType[wlType].executionTime;
        workloads[i].nvmeBandwidth = workloadsType[wlType].nvmeBandwidth;
        workloads[i].baseBandwidth=  workloadsType[wlType].baseBandwidth;
        workloads[i].nvmeCapacity = workloadsType[wlType].nvmeCapacity;
        workloads[i].wlId = i;
        workloads[i].performanceMultiplier=workloadsType[wlType].performanceMultiplier;
        workloads[i].limitPeakBandwidth=workloadsType[wlType].limitPeakBandwidth;
        workloads[i].cores = workloadsType[wlType].cores;

//        workloadsDistribution[i]["executionTime"] = workloadsType[wlType].executionTime;
//        workloadsDistribution[i]["nvmeBandwidth"] = workloadsType[wlType].nvmeBandwidth;
//        workloadsDistribution[i]["nvmeCapacity"] = workloadsType[wlType].nvmeCapacity;
    }
    //cout << workloadsDistribution.dump() << endl;

//    ArrivalUniformModel* arrival = new ArrivalUniformModel();
//    ArrivalPoissonModel* arrival = new ArrivalPoissonModel();
    ArrivalPoissonModelUniform* arrival = new ArrivalPoissonModelUniform();
//    MarkovModulatedpoissonProcess2* arrival = new MarkovModulatedpoissonProcess2(950,1200,0.1,0.9 );
//    MarkovModulatedpoissonProcess2* arrival = new MarkovModulatedpoissonProcess2(325,455,0.2,0.6 );
//    MarkovModulatedpoissonProcess2* arrival = new MarkovModulatedpoissonProcess2(200,350,0.1,0.8 );
//    MarkovModulatedpoissonProcess2* arrival = new MarkovModulatedpoissonProcess2(155.33,180,0.2,0.4 ); //PREFERRED
//    ArrivalRegularModel* arrival = new ArrivalRegularModel();
    //cluster experiments
//    arrival->generate_arrivals(workloads, 86400/patients, prio_threshold);
//    arrival->generate_arrivals(workloads, 3600/patients, prio_threshold);
    arrival->generate_arrivals(workloads, 72*60*60/patients, prio_threshold);
//    arrival->generate_arrivals(workloads, ((patients/40)*2194) / lambdaCoefficient, prio_threshold);
//    arrival->generate_arrivals(workloads, ((patients/40)*1778.37) / lambdaCoefficient, prio_threshold);
//    arrival->generate_arrivals(workloads, ((patients/40)*1500.37) / lambdaCoefficient, prio_threshold);
//    arrival->generate_arrivals(workloads, 40, prio_threshold);
//    arrival->generate_arrivals(workloads, 14, prio_threshold);
    //    arrival->generate_arrivals(workloads, ((patients/24)*1778.137) / lambdaCoefficient, prio_threshold);
    Layout layout = Layout();
    layout.generateLayout(layoutPath);
    DegradationModel* model = new DegradationModel();
    MinFragPolicy* minFrag = new MinFragPolicy(*model);
    QoSPolicy* qosPolicy = new QoSPolicy(*model);
    FirstFitPolicy* firstFit = new FirstFitPolicy(*model);
//    MinFragScheduler* scheduler = new MinFragScheduler();
    FcfsScheduler* fcfsSched = new FcfsScheduler();
    EarliestDeadlineStarvationScheduler* starvedf = new EarliestDeadlineStarvationScheduler(starvCoefficient);
    EarliestDeadlineStarvationSetsScheduler* setStarved = new EarliestDeadlineStarvationSetsScheduler(starvCoefficient);
    EarliestDeadlineScheduler* earliestSched = new EarliestDeadlineScheduler(starvCoefficient);
    EarliestSetDeadlineScheduler* earliestSetSched = new EarliestSetDeadlineScheduler(starvCoefficient);
//    cout << "bestfit: ";
//    simulator(scheduler, bestFit, workloads, patients, layout);
//    cout << "worstfit: ";
//    simulator(scheduler, worstFit, workloads, patients, layout);
//    cout << "randomfit: "
//    simulator(scheduler, randomFit, workloads, patients, layout);
//    cout << "minfrag: ";
    vector<workload> copyWL = workloads;
//    simulator(fcfsSched, firstFit, copyWL, patients, layout);
//    simulator(fcfsSched, minFrag, copyWL, patients, layout);
//    simulator(fcfsSched, qosPolicy, copyWL, patients, layout);
    simulator(earliestSched, firstFit, copyWL, patients, layout);
//    simulator(earliestSched, qosPolicy, copyWL, patients, layout);
//    simulator(setStarved, minFrag, copyWL, patients, layout);
//    simulator(setStarved, qosPolicy, copyWL, patients, layout);
//    simulator(starvedf, qosPolicy, copyWL, patients, layout);
//    simulator(setStarved, minFrag, copyWL, patients, layout);
//    simulator(earliestSched, minFrag, copyWL, patients, layout);//
//    simulator(earliestSetSched, minFrag, workloads, patients, layout);
//    simulator(earliestSetSched, qosPolicy, workloads, patients, layout);
//    simulator(fcfsSched, minFrag, copyWL, patients, layout);
//    cout << "worst fit: ";
//    simulator(fcfsSched, worstFit, workloads, patients, layout);
//    cout << "bestfit fcfs: ";
//    simulator(fcfsSched, bestFit, workloads, patients, layout);

    return 0;
}
