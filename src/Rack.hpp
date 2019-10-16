#ifndef RACK_H
#define RACK_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
#include "degradationModel.hpp"
using namespace std;

//class NvmeResource;


class Rack {
   public:
    int numFreeResources;
    int totalBandwidth;
    int totalCapacity;
    vector<NvmeResource> resources;
    vector<int> freeResources;
    vector<raid> compositions;
    int rackId;
    int freeCores = 0;
    int cores = 0;

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
    double getAvailableCapacity();
    double workloadsRaid();
    int compositionTTL(vector<workload>&, int, int);
    void setTotalBandwidth(int);
    void setTotalCapacity(int);
    void setFreeCores(int);
    void setTotalCores(int);
    int getTotalCores();
    bool possibleToColocate(vector<workload>&, int, int, int, DegradationModel&);
};

#endif
