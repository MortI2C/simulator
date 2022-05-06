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
    this->physicalGpu->assignWorkload(wload);
    wload->timeLeft = wload->executionTime;
//    this->wloads.push_back(wload);
    this->availMemory -= wload->gpuMemory;
    this->availBandwidth -= wload->gpuBandwidth;
    this->physicalGpu->setvGPUAsUsed(this);
    this->used = true;
    wload->allocation.vgpu = this;
}

bool vGPUResource::removeWorkload(workload* wload) {
    assert(this->physicalGpu->removeWorkload(wload));
    return true;
}

bool vGPUResource::isUsed() {
    return this->used;
}

GpuResource* vGPUResource::getPhysicalGpu() {
    return this->physicalGpu;
}

void vGPUResource::setUsed(bool used) {
    this->used = used;
}
