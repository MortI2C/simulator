#include <iostream>
#include <queue>
#include <vector>
#include "nvmeResource.hpp"
#include "resources_structures.hpp"

NvmeResource::NvmeResource (int totalBandwidth, int totalCapacity) {
    this->nvme.totalBandwidth = totalBandwidth;
    this->nvme.totalCapacity = totalCapacity;
    this->nvme.availableBandwidth = totalBandwidth;
    this->nvme.availableCapacity = totalCapacity;
}

int NvmeResource::getTotalBandwidth() {
    return this->nvme.totalBandwidth;
}

int NvmeResource::getTotalCapacity() {
    return this->nvme.totalCapacity;
}

int NvmeResource::getAvailableBandwidth() {
    return this->nvme.availableBandwidth;
}

int NvmeResource::getAvailableCapacity() {
    return this->nvme.availableCapacity;
}

void NvmeResource::setTotalBandwidth(int bandwidth) {
    this->nvme.totalBandwidth = bandwidth;
}

void NvmeResource::setTotalCapacity(int capacity) {
    this->nvme.totalCapacity = capacity;
}

void NvmeResource::setAvailableBandwidth(int bandwidth) {
    this->nvme.availableBandwidth = bandwidth;
}

void NvmeResource::setAvailableCapacity(int capacity) {
    this->nvme.availableCapacity = capacity;
}
