#include <iostream>
#include <queue>
#include <vector>
#include "gpuResource.hpp"
#include "resources_structures.hpp"

GpuResource::GpuResource (int totalBandwidth, int totalMemory) {
    this->totalBandwidth = totalBandwidth;
    this->totalMemory = totalMemory;
    this->availableBandwidth = totalBandwidth;
    this->availableMemory = totalMemory;
}

int GpuResource::getTotalBandwidth() {
    return this->totalBandwidth;
}

int GpuResource::getTotalMemory() {
    return this->totalMemory;
}

int GpuResource::getAvailableBandwidth() {
    return this->availableBandwidth;
}

int GpuResource::getAvailableMemory() {
    return this->availableMemory;
}

void GpuResource::setTotalBandwidth(int bandwidth) {
    this->totalBandwidth = bandwidth;
}

void GpuResource::setTotalMemory(int memory) {
    this->totalMemory = memory;
}

void GpuResource::setAvailableBandwidth(int bandwidth) {
    this->availableBandwidth = bandwidth;
}

void GpuResource::setAvailableMemory(int memory) {
    this->availableMemory = memory;
}

void GpuResource::setUsed(bool used) {
    this->used = used;
}

void GpuResource::assignWorkload(workload* wload) {
    this->workloads.push_back(wload);
}

bool GpuResource::isUsed() {
    return this->used;
}