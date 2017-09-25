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

struct allocatedResources {
    int composition;
    Rack* allocatedRack;
    int workloadsUsing = 0;
};

struct workload {
    allocatedResources allocation;
    int executionTime;
    int nvmeBandwidth;
    int nvmeCapacity;
    bool highprio;
    int deadline;
    int arrival;
    int scheduled;
    int cyclesDelayed=0;
};

#endif
