#include <iostream>
#include <queue>
#include <vector>
#include "gpuResource.hpp"
#include "resources_structures.hpp"

GpuResource::GpuResource (int totalBandwidth, int totalMemory) {
    this->gpu.totalBandwidth = totalBandwidth;
    this->gpu.totalMemory = totalMemory;
    this->gpu.availableBandwidth = totalBandwidth;
    this->gpu.availableMemory = totalMemory;
}

int GpuResource::getTotalBandwidth() {
    return this->gpu.totalBandwidth;
}

int GpuResource::getTotalMemory() {
    return this->gpu.totalMemory;
}

int GpuResource::getAvailableBandwidth() {
    return this->gpu.availableBandwidth;
}

int GpuResource::getAvailableMemory() {
    return this->gpu.availableMemory;
}

void GpuResource::setTotalBandwidth(int bandwidth) {
    this->gpu.totalBandwidth = bandwidth;
}

void GpuResource::setTotalMemory(int memory) {
    this->gpu.totalMemory = memory;
}

void GpuResource::setAvailableBandwidth(int bandwidth) {
    this->gpu.availableBandwidth = bandwidth;
}

void GpuResource::setAvailableMemory(int memory) {
    this->gpu.availableMemory = memory;
}
