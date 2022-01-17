#ifndef VGPU_RESOURCE_H
#define VGPU_RESOURCE_H
#include <iostream>
#include <vector>
#include <queue>
#include "gpuResource.hpp"

struct vGPU {
    GpuResource* physicalGpu;
    int totalMemory;
    int totalBandwidth;
    bool used = false;
};

class vGPUResource {
    vGPU vGpu;

    public:
        vGPUResource() {

		};
        vGPUResource (int, int, GpuResource*);
		void setTotalMemory(int);
		void setTotalBandwidth(int);
		int getTotalMemory();
		int getTotalBandwidth();
};
#endif