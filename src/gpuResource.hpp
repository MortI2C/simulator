#ifndef GPU_RESOURCE_H
#define GPU_RESOURCE_H
#include <iostream>
#include <vector>
#include <queue>
#include "resources_structures.hpp"

class GpuResource {
    int availableBandwidth;
    int availableMemory;
    int totalBandwidth;
    int totalMemory;
    bool used = false;
    vector<workload*> workloads;

    public:
        GpuResource() {

		};
        GpuResource (int, int);
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
};
#endif