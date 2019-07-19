#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <typeinfo>
#include <algorithm>
#include <string>
#include "nlohmann/json.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
#include "placementPolicy.hpp"
#include "mmpp-2.hpp"
#include "qosPolicy.hpp"
#include "firstFit.hpp"
#include "randomPolicy.hpp"
#include "fcfsSchedulePolicy.hpp"
#include "degradationModel.hpp"
#include "layout.hpp"

using namespace std;

const int BASE_TIME = 1587;

double getAvgCompletionTime(int step, const vector<workload> &scheduledWorkloads)
{
    double completionTime = 0;
    for (auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it)
    {
        completionTime += (it->stepFinished - it->arrival);
    }
    completionTime /= scheduledWorkloads.size();

    return completionTime;
}

double getAvgExeTime(int step, const vector<workload> &scheduledWorkloads)
{
    double exeTime = 0;
    for (auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it)
    {
        exeTime += (it->stepFinished - it->scheduled);
    }
    exeTime /= scheduledWorkloads.size();

    return exeTime;
}

double getAvgWaitingTime(int step, const vector<workload> &scheduledWorkloads)
{
    double waitingTime = 0;
    for (auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it)
    {
        waitingTime += (it->scheduled - it->arrival);
    }
    waitingTime /= scheduledWorkloads.size();

    return waitingTime;
}

void printStatistics(int step, const vector<workload> &scheduledWorkloads, int stationaryStep = 0)
{
    int waitingTime = 0;
    int exeTime = 0;
    int completionTime = 0;
    int missedDeadlines = 0;
    int workloadsInStationary = 0;
    for (auto it = scheduledWorkloads.begin(); it != scheduledWorkloads.end(); ++it)
    {
        if (stationaryStep <= it->arrival)
        {
            waitingTime += (it->scheduled - it->arrival);
            exeTime += (it->stepFinished - it->scheduled);
            completionTime += (it->stepFinished - it->arrival);
            if (it->stepFinished > it->deadline)
                ++missedDeadlines;
            workloadsInStationary++;
        }
    }
    waitingTime /= workloadsInStationary;
    exeTime /= workloadsInStationary;
    completionTime /= workloadsInStationary;

    cout << step << " " << exeTime << " " << completionTime << " " << waitingTime << " " << missedDeadlines << endl;
}

void simulator(SchedulingPolicy *scheduler, PlacementPolicy *placementPolicy, vector<workload> &workloads, int patients, Layout &layout)
{
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
    vector<workload>::iterator wlpointer = workloads.begin();
    while (processedPatients < patients || !runningWorkloads.empty())
    {
        //1st Check Workloads running finishing in this step
        vector<int> toFinish;
        for (auto it = runningWorkloads.begin(); it != runningWorkloads.end(); ++it)
        {
            workload *run = &workloads[*it];
            run->timeLeft--;
            if (run->timeLeft <= 0)
            {
                workloads[*it].stepFinished = step;
                scheduler->logger[*it]["completion"] = step;
                toFinish.push_back(*it);
                placementPolicy->freeResources(workloads, *it);
                scheduler->logger[*it]["completionLoadFactor"] = layout.loadFactor(workloads, pendingToSchedule, runningWorkloads);
            }
        }

        //Remove already placed workloads
        for (auto it = toFinish.begin(); it != toFinish.end(); ++it)
        {
            for (auto it2 = runningWorkloads.begin(); it2 != runningWorkloads.end(); ++it2)
            {
                if (*it2 == *it)
                {
                    runningWorkloads.erase(it2);
                    break;
                }
            }
        }

        //2nd add arriving workloads to pending to schedule
        while (wlpointer != workloads.end() && wlpointer->arrival <= step)
        {
            pendingToSchedule.push_back(wlpointer->wlId);
            ++wlpointer;
        }

        //3rd schedule new workloads
        //Postcondition: workloads to run inserted in vector in completion step order!
        int priorScheduler = pendingToSchedule.size();
        scheduler->scheduleWorkloads(workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
        processedPatients += (priorScheduler - pendingToSchedule.size());
        if (stationaryStep == -1 && layout.resourcesUsed() >= 0.9)
            stationaryStep = step;

        double currResourcesUsed = layout.resourcesUsed();
        double currFrag = layout.calculateFragmentation();
        double currLoadFactor = layout.loadFactor(workloads, pendingToSchedule, runningWorkloads);
        double currActLoadFactor = layout.actualLoadFactor(workloads, runningWorkloads);
        if (stationaryStep >= 0)
        {
            frag += currFrag;
            resourcesUsed += currResourcesUsed;
            loadFactor += currLoadFactor;
            actualLoadFactor += currActLoadFactor;
        }

        placementPolicy->loadFactor = currLoadFactor;
        int raids = layout.raidsUsed();
        double size = layout.avgRaidSize();
        ++step;
    }
    step--; //correction

    printStatistics(step, workloads, stationaryStep);
    //To output simulation trace (to generate plots)
//    cout << scheduler->logger.dump() << endl;
}

int main(int argc, char *argv[])
{
    int patients = 1500;
    if (argc > 1)
        patients = atoi(argv[1]);

    string layoutPath = "layouts/layout-1.json";
    if (argc > 2)
        layoutPath = argv[2];

    double prio_threshold = 0.2;
    if (argc > 3)
        prio_threshold = atof(argv[3]);

//    WorkloadPoissonGenerator* generator = new WorkloadPoissonGenerator();
    vector<workload> workloads(patients);
    for (int i = 0; i < patients; ++i)
    {
        workloads[i].executionTime = 1590;
        workloads[i].nvmeBandwidth = 423.615;
        workloads[i].nvmeCapacity = 43;
        workloads[i].wlId = i;
    }

    MarkovModulatedpoissonProcess2 *arrival = new MarkovModulatedpoissonProcess2(20, 50, 0.1, 2);
    arrival->generate_arrivals(workloads, 14, prio_threshold);

    Layout layout = Layout();
    layout.generateLayout(layoutPath);
    DegradationModel *model = new DegradationModel();
    QoSPolicy *qosPolicy = new QoSPolicy(*model);
    FirstFitPolicy *firstFit = new FirstFitPolicy(*model);
    FcfsScheduler *fcfsSched = new FcfsScheduler();
    RandomScheduler *randomScheduler = new RandomScheduler();
    vector<workload> copyWL = workloads;
    // simulator(fcfsSched, qosPolicy, copyWL, patients, layout);
    simulator(randomScheduler, firstFit, copyWL, patients, layout);

    return 0;
}
