#ifndef RACK_H
#define RACK_H
#include <iostream>
#include <vector>
//#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

//class NvmeResource;

struct raid {
    NvmeResource composedNvme;
    vector<int> volumes;
    bool used = false;
    int workloadsUsing = 0;
    vector<int> assignedWorkloads;
};

class Rack {
   public:
    int numFreeResources;
    int totalBandwidth;
    int totalCapacity;
    vector<NvmeResource> resources;
    vector<int> freeResources;
    vector<raid> compositions;
    int rackId;

    Rack() {

    }
    void addNvmeResource(NvmeResource&);
    void deleteNvmeResource (NvmeResource*);
    void freeComposition(Rack*, int);
    void dumpRack();
    void addNvmeResourceVector(vector<NvmeResource>);
    double calculateFragmentation();
    double estimateFragmentation(int, int, int);
    int getTotalCapacityUsed();
    int getTotalBandwidthUsed();
    bool inUse();
    double resourcesUsed();
    double workloadsRaid();
    int compositionTTL(vector<workload>&, int, int);
    void setTotalBandwidth(int);
    void setTotalCapacity(int);
};

#endif
