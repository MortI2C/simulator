#include <iostream>
#include <queue>
#include <vector>
#include "nvmeResource.hpp"
#include "resources_structures.hpp"

NvmeResource::NvmeResource (std::vector<nvme_slot> resource) {
    this->nvme = resource;
}

bool NvmeResource::availableSlots() {
    bool avail = false;
    for(std::vector<nvme_slot>::iterator it = nvme.begin(); !avail && it!=nvme.end(); ++it)
        avail = !it->used;
    return avail;
}

std::vector<nvme_slot>::iterator NvmeResource::getSlot() {
    std::vector<nvme_slot>::iterator found = nvme.end();
    for(std::vector<nvme_slot>::iterator it = nvme.begin(); found==nvme.end() && it!=nvme.end(); ++it)
        if(!it->used) found = it;

    return found;
}
