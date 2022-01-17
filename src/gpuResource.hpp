#ifndef GPU_RESOURCE_H
#define GPU_RESOURCE_H
#include <iostream>
#include <vector>
#include <queue>

struct Gpu {
    int availableBandwidth;
    int availableMemory;
    int totalBandwidth;
    int totalMemory;
    bool used = false;
};

class GpuResource {
    Gpu gpu;

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
};
#endif