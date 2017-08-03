#ifndef QOSDEDICATEDPOLICY_H
#define QOSDEDICATEDPOLICY_H

#include <iostream>
#include <vector>
#include <queue>
#include "policy.hpp"
#include "nvmeResource.hpp"
#include "resources_structures.hpp"

class QoSPolicy : public Policy {
    public:
		QoSPolicy() {
		}
		void initializeResources();
		void freeResources(int);
		bool scheduleWorkload(vector<workload>::iterator, int);
	private:
		NvmeResource sharedNvme;
		NvmeResource restricted;

		vector<allocated_resources> allocated;

		bool availableResources() {
			return sharedNvme.availableSlots() || restricted.availableSlots();
		}
};

#endif
