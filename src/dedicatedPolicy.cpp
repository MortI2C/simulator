#include <iostream>
#include <vector>
#include <queue>
#include "policy.hpp"
#include "dedicatedPolicy.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

void DedicatedPolicy::initializeResources() {
    vector<nvme_slot> sharedNvme;
    sharedNvme.push_back({1951, false});
    sharedNvme.push_back({1960, false});
    sharedNvme.push_back({2090, false});
    sharedNvme.push_back({2244, false});

    vector<nvme_slot> dedicated;
    dedicated.push_back({1600, false});

    this->sharedNvme = NvmeResource(sharedNvme);
    this->dedicated = NvmeResource(dedicated);
}

bool DedicatedPolicy::scheduleWorkload(vector<workload>::iterator wlpointer, int step) {
    if(availableResources()) {
        vector<nvme_slot>::iterator slot;
        if(wlpointer->highprio && this->dedicated.availableSlots())
            slot = this->dedicated.getSlot();
        else if(wlpointer->highprio && this->sharedNvme.availableSlots())
            slot =this->sharedNvme.getSlot();
        else if(this->sharedNvme.availableSlots())
            slot = this->sharedNvme.getSlot();
        else
            slot = this->dedicated.getSlot();

        wlpointer->completion_time = slot->time+step;
        allocated.push_back({slot, wlpointer->completion_time});
        slot->used = true;
        wlpointer->scheduled = step;

        return true;
    } else
        return false;
}

void DedicatedPolicy::freeResources(int step) {
    vector<vector<allocated_resources>::iterator > foundElems;
    for(vector<allocated_resources>::iterator el = this->allocated.begin(); el!=this->allocated.end(); ++el) {
        if(el->completion_time <= step) {
            foundElems.push_back(el);
            el->resource->used = false;
        }
    }

    for(vector<vector<allocated_resources>::iterator >::iterator it = foundElems.begin(); it!=foundElems.end(); ++it)
        this->allocated.erase(*it);
}