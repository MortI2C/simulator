#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
#include "json.hpp"
using namespace std;
using json = nlohmann::json;

void SchedulingPolicy::log(int workloadId, vector<workload>& workloads,
                           vector <int>& pendingToSchedule,
                           vector <int>& runningWorkloads,
                           PlacementPolicy* placementPolicy, int step, Layout& layout) {
    this->logger[step]["step"] = step;
    this->logger[step]["jobid"] = workloadId;
    this->logger[step]["rackid"] = workloads[workloadId].allocation.allocatedRack->rackId;
    this->logger[step]["volumes"] = workloads[workloadId].allocation.allocatedRack->compositions[workloads[workloadId].allocation.composition].volumes;
    this->logger[step]["loadFactor"] = layout.loadFactor(workloads, pendingToSchedule,runningWorkloads);
    this->logger[step]["deadline"] = workloads[workloadId].deadline;
    this->logger[step]["arrival"] = workloads[workloadId].arrival;
    this->logger[step]["scheduled"] = workloads[workloadId].scheduled;
}
