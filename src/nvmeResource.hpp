#ifndef NVME_RESOURCE_H
#define NVME_RESOURCE_H
#include <iostream>
#include <vector>
#include <queue>
#include "resources_structures.hpp"

class NvmeResource {
    std::vector<nvme_slot> nvme;

    public:
		NvmeResource() {

		};
		NvmeResource (std::vector<nvme_slot>);
		bool availableSlots();
		std::vector<nvme_slot>::iterator getSlot();
};
#endif