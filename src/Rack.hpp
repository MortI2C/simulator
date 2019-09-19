#ifndef RACK_H
#define RACK_H
#include <iostream>
#include <vector>
//#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

//class NvmeResource;

struct raid {
    raid() : composedNvme(NvmeResource(0,0)), volumes(vector<int>(0)), used(false), workloadsUsing(0), assignedWorkloads(vector<int>(0)) {}
    NvmeResource composedNvme;
    vector<int> volumes;
    bool used;
    int workloadsUsing;
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
    double getAvailableBandwidth();
    double workloadsRaid();
    int compositionTTL(vector<workload>&, int, int);
    void setTotalBandwidth(int);
    void setTotalCapacity(int);
};

#endif
