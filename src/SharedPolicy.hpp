#ifndef SHAREDPOLICY_H
#define SHAREDPOLICY_H

#include <iostream>
#include <vector>
#include <queue>
#include "policy.hpp"
#include "nvmeResource.hpp"
#include "resources_structures.hpp"

class SharedPolicy : public Policy {
    public:
		SharedPolicy() {
		}
		void initializeResources(int);
		void freeResources(int);
		bool scheduleWorkload(vector<workload>::iterator, int);
	private:
		NvmeResource nvme;

		vector<allocated_resources> allocated;

		bool availableResources() {
			return nvme.availableSlots();
		}
};

#endif
