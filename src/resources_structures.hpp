#ifndef RESOURCES_STRUCTURES_H
#define RESOURCES_STRUCTURES_H

#include <iostream>
#include <vector>
#include "math.h"
#include "nvmeResource.hpp"
//#include "Rack.hpp"
using namespace std;

class Rack;

struct raid {
    raid() : composedNvme(NvmeResource(0,0)), volumes(vector<int>(0)), used(false), workloadsUsing(0), assignedWorkloads(vector<int>(0)) {}
    NvmeResource composedNvme;
    vector<int> volumes;
    bool used = false;
    int workloadsUsing;
    vector<int> assignedWorkloads;
    Rack* coresRack;
};

struct loadFactors {
    double bandwidthLF;
    double capacityLF;
    double cpuLF;
};

struct rackFitness {
    int fitness;
    bool inUse;
    vector<int> selection;
    Rack* rack;
};

struct nvmeFitness {
    int fitness;
    int ttlDifference;
    int composition;
    Rack* rack;
};

struct allocatedResources {
    allocatedResources() : composition(-1) {}
    int composition;
    Rack* allocatedRack;
    Rack* coresAllocatedRack;
};

struct workload {
    allocatedResources allocation;
    int executionTime;
    int baseExecutionTime;
    int timeLeft;
    int nvmeBandwidth;
    int nvmeCapacity;
    bool highprio;
    int deadline;
    int arrival;
    int scheduled;
    int stepFinished;
    int cyclesDelayed=0;
    int wlId;
    double performanceMultiplier;
    int baseBandwidth;
    int limitPeakBandwidth;
    int cores;
    string wlName;
    int allocationAttempts = 0;
    int failToAllocateDueCores = 0;
};

#endif
