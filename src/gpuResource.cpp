#include <iostream>
#include <queue>
#include <vector>
#include <cassert>
#include "gpuResource.hpp"
#include "vgpuResource.hpp"
#include "resources_structures.hpp"

GpuResource::GpuResource (int totalBandwidth, int totalMemory) {
    this->totalBandwidth = totalBandwidth;
    this->totalMemory = totalMemory;
    this->availableBandwidth = totalBandwidth;
    this->availableMemory = totalMemory;
}

void GpuResource::addVgpusVector(vector<vGPUResource*> vgpus) {
    this->vgpus = vgpus;
    this->usedVgpus = vector<int>(vgpus.size(),0);
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
    this->availableBandwidth -= wload->gpuBandwidth;
    this->availableMemory -= wload->gpuMemory;
}

bool GpuResource::isUsed() {
    return this->used;
}

bool GpuResource::removeWorkload(workload* wload) {
    bool found = false;
    for(auto it = this->workloads.begin(); !found && it!=this->workloads.end(); ++it) {
        if(wload->wlId == (*it)->wlId) {
            found = true;
            this->workloads.erase(it);
            this->availableBandwidth += wload->gpuBandwidth;
            this->availableMemory += wload->gpuMemory;
        }
    }
    return found;
}

vGPUResource* GpuResource::possibleAllocateWloadInvGPU(int bandwidth, int memory) {
    int element = 0;
    for(auto it = this->usedVgpus.begin(); it!=this->usedVgpus.end(); ++it, ++element) {
        if(!*it) {
            vGPUResource* vgpu = this->vgpus[element];
            if(vgpu->getAvailableBandwidth() >= bandwidth && vgpu->getAvailableMemory() >= memory)
                return vgpu;
        }
    }

    return nullptr;
}

void GpuResource::setvGPUUsed(vGPUResource* vgpu) {
    bool found = false;
    int element = 0;
    for(auto it = this->vgpus.begin(); !found && it!=this->vgpus.end(); ++it, ++element) {
        if(*it == vgpu) {
            found = true;
            this->usedVgpus[element] = 1;
            this->used = true;
            this->vgpusUsed++;
        }
    }
    if(!found) {
        cerr << "GPU NOT FOUND" << endl;
        assert(0==1);
    }
}

void GpuResource::freevGPU(vGPUResource * vgpu) {
    bool found = false;
    int element = 0;
    for(auto it = this->vgpus.begin(); !found && it!=this->vgpus.end(); ++it, ++element) {
        if(*it == vgpu) {
            found = true;
            this->usedVgpus[element] = 0;
            this->vgpusUsed--;
        }
    }

    if(!found) {
        cerr << "GPU NOT FOUND" << endl;
        assert(0==1);
    } else if (this->vgpusUsed == 0) {
        this->vgpus.clear();
        this->usedVgpus.clear();
        this->used = false;
    }
}

void GpuResource::setvGPUAsUsed(vGPUResource* vgpu) {
    bool found = false;
    int element = 0;
    for(auto it = this->vgpus.begin(); !found && it!=this->vgpus.end(); ++it, ++element) {
        if(*it == vgpu) {
            found = true;
            this->usedVgpus[element] = 1;
            this->vgpusUsed++;
        }
    }

    assert(found);
}