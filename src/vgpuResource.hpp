#ifndef VGPU_RESOURCE_H
#define VGPU_RESOURCE_H
#include <iostream>
#include <vector>
#include <queue>
#include "gpuResource.hpp"
#include "resources_structures.hpp"

class vGPUResource {
    GpuResource* physicalGpu;
    int totalMemory;
    int totalBandwidth;
    int availMemory;
    int availBandwidth;
    bool used = false;
    vector<workload*> wloads;

public:
        vGPUResource() {

		};
        vGPUResource (int, int, GpuResource*);
		void setTotalMemory(int);
		void setTotalBandwidth(int);
		int getTotalMemory();
		int getTotalBandwidth();
		vector<workload*> getWorkloads();
		int getAvailableMemory();
		int getAvailableBandwidth();
		void assignWorkload(workload*);
		bool removeWorkload(workload*);
		bool isUsed();
		void setUsed(bool);
		GpuResource* getPhysicalGpu();
		int getNumberWorkloads() {
		    return this->wloads.size();
		}
};
#endif