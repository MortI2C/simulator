#ifndef RACK_H
#define RACK_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
#include "degradationModel.hpp"
#include "vgpuResource.hpp"
#include "gpuResource.hpp"
using namespace std;

//class NvmeResource;


class Rack {
   public:
    int numFreeResources;
    int totalBandwidth;
    int totalCapacity;
    int totalGpuBandwidth = 0;
    int totalGpuMemory = 0;
    vector<GpuResource> gpus;
    vector<vGPUResource*> vgpus;
//    vGPUResource* vgpu = nullptr;
    vector<NvmeResource> resources;
    vector<int> freeResources;
    vector<raid> compositions;
    int rackId;
    int freeCores = 0;
    int cores = 0;

    Rack() {
    }
    void addGpuResourceVector(vector<GpuResource>);
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
    void setTotalGpuBandwidth(int);
    void setTotalGpuMemory(int);
    int getTotalCores();
    bool possibleToColocate(vector<workload>&, int, int, int, DegradationModel&);
    void addvGPU(vGPUResource* vGPU);
    vector<GpuResource>::iterator possiblePhysGPUAllocation(int, int);
//    void assignWorkloadTovGPU(workload*);
};

#endif
