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
    int numVolumes = 0;
    bool used = false;
    int workloadsUsing = 0;
    vector<int> assignedWorkloads;
};

class Rack {
   public:
    int numFreeResources;
    vector<NvmeResource> resources;
    vector<int> freeResources;
    vector<raid> compositions;

    Rack() {

    }
    void addNvmeResource(NvmeResource&);
    void deleteNvmeResource (NvmeResource*);
    void freeComposition(Rack*, int);
    void dumpRack();
    void stabilizeContainers();
    void addNvmeResourceVector(vector<NvmeResource>);
    double calculateFragmentation();
    double estimateFragmentation(int, int, int);
    int getTotalCapacityUsed();
    int getTotalBandwidthUsed();
    bool inUse();
    double resourcesUsed();
    double workloadsRaid();
    int compositionTTL(vector<workload>&, int, int);
};

#endif
