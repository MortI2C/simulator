#include <iostream>
#include <queue>
#include <vector>
#include <assert.h>
#include "vgpuResource.hpp"
#include "gpuResource.hpp"
#include "resources_structures.hpp"

vGPUResource::vGPUResource (int bandwidth, int memory, GpuResource* physGpu) {
    assert(memory <= physGpu->getTotalMemory() && bandwidth <= physGpu->getTotalBandwidth());
    this->physicalGpu = physGpu;
    this->totalBandwidth = bandwidth;
    this->totalMemory = memory;
    this->availBandwidth = bandwidth;
    this->availMemory = memory;
}

int vGPUResource::getTotalMemory() {
    return this->totalMemory;
}


int vGPUResource::getTotalBandwidth() {
    return this->totalBandwidth;
}

void vGPUResource::setTotalBandwidth(int bandwidth) {
    this->totalBandwidth = bandwidth;
}

void vGPUResource::setTotalMemory(int memory) {
    this->totalMemory = memory;
}

vector<workload*> vGPUResource::getWorkloads() {
    return this->wloads;
}

int vGPUResource::getAvailableBandwidth() {
    return this->availBandwidth;
}

int vGPUResource::getAvailableMemory() {
    return this->availMemory;
}

void vGPUResource::assignWorkload(workload* wload) {
    this->wloads.push_back(wload);
    this->availMemory -= wload->gpuMemory;
    this->availBandwidth -= wload->gpuBandwidth;
    this->physicalGpu->setvGPUAsUsed(this);
}

bool vGPUResource::removeWorkload(workload* wload) {
    bool found = false;
    for(auto it = this->wloads.begin(); !found && it!=this->wloads.end(); ++it) {
        if(wload->wlId == (*it)->wlId) {
            found = true;
            this->wloads.erase(it);
            this->availMemory += wload->gpuMemory;
            this->availBandwidth += wload->gpuBandwidth;
        }
    }
    assert(this->wloads.size()>=0);
    if(this->wloads.size()==0) {
        this->used = false;
        this->physicalGpu->freevGPU(this);
    }

    return found;
}

bool vGPUResource::isUsed() {
    return this->wloads.size()>0;
}

GpuResource* vGPUResource::getPhysicalGpu() {
    return this->physicalGpu;
}
