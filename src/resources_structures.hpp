#ifndef RESOURCES_STRUCTURES_H
#define RESOURCES_STRUCTURES_H

#include <iostream>
#include <vector>
#include "math.h"
#include "nvmeResource.hpp"
//#include "Rack.hpp"
using namespace std;

class Rack;
struct raid;

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
    int composition;
    Rack* allocatedRack;
};

struct workload {
    allocatedResources allocation;
    int executionTime;
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
};

#endif
