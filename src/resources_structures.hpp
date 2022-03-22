#ifndef RESOURCES_STRUCTURES_H
#define RESOURCES_STRUCTURES_H

#include <iostream>
#include <vector>
#include "math.h"
#include "nvmeResource.hpp"

//#include "Rack.hpp"
using namespace std;

class Rack;
class GpuResource;
class vGPUResource;

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
    double gpuMemLF;
};

struct rackFitness {
    int fitness;
    bool inUse;
    vector<int> selection;
    Rack* rack;
    GpuResource* gpu = nullptr;
    vGPUResource* vgpu = nullptr;
    rackFitness(int f,bool i,vector<int> s,Rack* r) {
        this->fitness = f;
        this->inUse = i;
        this->selection = s;
        this->rack = r;
    }
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
    vGPUResource* vgpu = nullptr;
};

struct workload {
    allocatedResources allocation;
    int executionTime;
    int baseExecutionTime;
    int timeLeft;
    int nvmeBandwidth = 0;
    int nvmeCapacity = 0;
    int gpuBandwidth = 0;
    int gpuMemory = 0;
    bool highprio;
    int deadline;
    int arrival;
    int scheduled;
    int stepFinished;
    int cyclesDelayed=0;
    int wlId;
    double performanceMultiplier;
    int baseBandwidth = 0;
    int limitPeakBandwidth;
    int cores;
    string wlName;
    string wlType;
    int allocationAttempts = 0;
    int failToAllocateDueCores = 0;
    string placementPolicy;
};

#endif
