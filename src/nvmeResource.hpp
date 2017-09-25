#ifndef NVME_RESOURCE_H
#define NVME_RESOURCE_H
#include <iostream>
#include <vector>
#include <queue>
#include "resources_structures.hpp"

class NvmeResource {
    struct nvme {
		int availableBandwidth;
		int availableCapacity;
		int totalBandwidth;
		int totalCapacity;
		bool used = false;
	} nvme;

    public:
		NvmeResource() {

		};
		NvmeResource (int, int);
		void setTotalBandwidth(int);
		void setTotalCapacity(int);
		void setAvailableBandwidth(int);
		void setAvailableCapacity(int);
		int getTotalBandwidth();
		int getTotalCapacity();
		int getAvailableBandwidth();
		int getAvailableCapacity();
};
#endif