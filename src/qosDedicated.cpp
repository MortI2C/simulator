#include <iostream>
#include <vector>
#include <queue>
#include "policy.hpp"
#include "qosDedicated.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

void QoSPolicy::initializeResources() {
    vector<nvme_slot> sharedNvme;
    sharedNvme.push_back({1564, false});
    sharedNvme.push_back({1571, false});
    sharedNvme.push_back({1588, false});
    sharedNvme.push_back({1575, false});

    vector<nvme_slot> restricted;
    restricted.push_back({1926, false});

    this->sharedNvme = NvmeResource(sharedNvme);
    this->restricted = NvmeResource(restricted);
}

bool QoSPolicy::scheduleWorkload(vector<workload>::iterator wlpointer, int step) {
    if(availableResources()) {
        vector<nvme_slot>::iterator slot;
        if(!wlpointer->highprio && this->restricted.availableSlots())
            slot = this->restricted.getSlot();
        else if(wlpointer->highprio && this->sharedNvme.availableSlots())
            slot =this->sharedNvme.getSlot();
        else if(this->sharedNvme.availableSlots())
            slot = this->sharedNvme.getSlot();
        else
            slot = this->restricted.getSlot();

        wlpointer->completion_time = slot->time+step;
        allocated.push_back({slot, wlpointer->completion_time});
        slot->used = true;
        wlpointer->scheduled = step;

        return true;
    } else
        return false;
}

void QoSPolicy::freeResources(int step) {
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