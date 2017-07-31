#ifndef DEDICATEDPOLICY_H
#define DEDICATEDPOLICY_H

#include <iostream>
#include <vector>
#include <queue>
#include "policy.hpp"
#include "nvmeResource.hpp"
#include "resources_structures.hpp"

class DedicatedPolicy : public Policy {
    public:
		DedicatedPolicy() {
		}
		void initializeResources();
		void freeResources(int);
		bool scheduleWorkload(vector<workload>::iterator, int);
	private:
		NvmeResource sharedNvme;
		NvmeResource dedicated;

		vector<allocated_resources> allocated;

		bool availableResources() {
			return sharedNvme.availableSlots() || dedicated.availableSlots();
		}
};

#endif
