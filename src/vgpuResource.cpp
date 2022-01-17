#include <iostream>
#include <queue>
#include <vector>
#include <assert.h>
#include "vgpuResource.hpp"
#include "gpuResource.hpp"
#include "resources_structures.hpp"

vGPUResource::vGPUResource (int memory, int bandwidth, GpuResource* physGpu) {
    assert(memory <= physGpu->getTotalMemory() && bandwidth <= physGpu->getTotalBandwidth());
    this->vGpu.physicalGpu = physGpu;
    this->vGpu.totalBandwidth = bandwidth;
    this->vGpu.totalMemory = memory;
}

int vGPUResource::getTotalMemory() {
    return this->vGpu.totalMemory;
}


int vGPUResource::getTotalBandwidth() {
    return this->vGpu.totalBandwidth;
}

void vGPUResource::setTotalBandwidth(int bandwidth) {
    this->vGpu.totalBandwidth = bandwidth;
}

void vGPUResource::setTotalMemory(int memory) {
    this->vGpu.totalMemory = memory;
}
