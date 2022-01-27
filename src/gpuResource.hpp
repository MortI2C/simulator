#ifndef GPU_RESOURCE_H
#define GPU_RESOURCE_H
#include <iostream>
#include <vector>
#include <queue>
#include "gpuResource.hpp"
#include "resources_structures.hpp"

class vGPUResource;

class GpuResource {
    int availableBandwidth;
    int availableMemory;
    int totalBandwidth;
    int totalMemory;
    bool used = false;
    vector<workload*> workloads;
    vector<vGPUResource*> vgpus;
    vector<int> usedVgpus;
    int vgpusUsed = 0;

    public:
        GpuResource() {

		};
        GpuResource (int, int);
        void addVgpusVector(vector<vGPUResource*>);
		void setTotalBandwidth(int);
		void setTotalMemory(int);
		void setAvailableBandwidth(int);
		void setAvailableMemory(int);
		int getTotalBandwidth();
		int getTotalMemory();
		int getAvailableBandwidth();
		int getAvailableMemory();
		void setUsed(bool);
		void assignWorkload(workload*);
		bool isUsed();
        bool removeWorkload(workload*);
        void setvGPUUsed(vGPUResource*);
        void freevGPU(vGPUResource*);
        vGPUResource* possibleAllocateWloadInvGPU(int, int);
        void setvGPUAsUsed(vGPUResource*);
};
#endif