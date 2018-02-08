#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
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
//    if(this->logger[step]["step"] != nullptr)
//        cerr << "exists!" << endl;
    for(auto it =workloads[workloadId].allocation.allocatedRack->compositions[workloads[workloadId].allocation.composition].volumes.begin();
            it!=workloads[workloadId].allocation.allocatedRack->compositions[workloads[workloadId].allocation.composition].volumes.end();
            ++it) {
        string newVolume = to_string(workloads[workloadId].allocation.allocatedRack->rackId) + to_string(*it);
        if(find(this->logger[step]["volumes"].begin(),this->logger[step]["volumes"].end(),newVolume) == this->logger[step]["volumes"].end())
            this->logger[step]["volumes"].push_back(newVolume);
    }

    this->logger[step]["step"] = step;
    this->logger[step]["jobid"].push_back(workloadId);
    this->logger[step]["rackid"].push_back(workloads[workloadId].allocation.allocatedRack->rackId);
    if(layout.loadFactor(workloads, pendingToSchedule,runningWorkloads) > this->logger[step]["loadFactor"])
        this->logger[step]["loadFactor"] = layout.loadFactor(workloads, pendingToSchedule,runningWorkloads);
    this->logger[step]["deadline"].push_back(workloads[workloadId].deadline);
    this->logger[step]["arrival"].push_back(workloads[workloadId].arrival);
    this->logger[step]["scheduled"].push_back(workloads[workloadId].scheduled);
}
