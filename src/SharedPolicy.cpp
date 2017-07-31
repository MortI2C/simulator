#include <iostream>
#include <vector>
#include <queue>
#include "policy.hpp"
#include "SharedPolicy.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

void SharedPolicy::initializeResources(int numnvme = 2) {
    vector <nvme_slot> sharedNvme;
    if(numnvme == 2) {
        sharedNvme.push_back({1644, false});
        sharedNvme.push_back({1638, false});
        sharedNvme.push_back({1627, false});
        sharedNvme.push_back({1627, false});
        sharedNvme.push_back({1618, false});
    } else {
        sharedNvme.push_back({2450, false});
        sharedNvme.push_back({2385, false});
        sharedNvme.push_back({2136, false});
        sharedNvme.push_back({2412, false});
        sharedNvme.push_back({2487, false});
    }

    this->nvme = NvmeResource(sharedNvme);
}

bool SharedPolicy::scheduleWorkload(vector<workload>::iterator wlpointer, int step) {
    if(availableResources()) {
        vector<nvme_slot>::iterator slot = this->nvme.getSlot();

        wlpointer->completion_time = slot->time+step;
        allocated.push_back({slot, wlpointer->completion_time});
        slot->used = true;
        wlpointer->scheduled = step;

        return true;
    } else
        return false;
}

void SharedPolicy::freeResources(int step) {
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