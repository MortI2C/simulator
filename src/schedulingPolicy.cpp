#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "schedulingPolicy.hpp"
#include "nlohmann/json.hpp"
using namespace std;
using json = nlohmann::json;

void SchedulingPolicy::log(int workloadId, vector<workload>& workloads,
                           vector <int>& pendingToSchedule,
                           vector <int>& runningWorkloads,
                           PlacementPolicy* placementPolicy, int step, Layout& layout) {
//    if(this->logger[workloadId]["step"] != nullptr)
//        cerr << "exists!" << endl;
    string newVolume = "";
    for(auto it =workloads[workloadId].allocation.allocatedRack->compositions[workloads[workloadId].allocation.composition].volumes.begin();
            it!=workloads[workloadId].allocation.allocatedRack->compositions[workloads[workloadId].allocation.composition].volumes.end();
            ++it) {
        newVolume = newVolume + to_string(*it); //to_string(workloads[workloadId].allocation.allocatedRack->rackId) +
    }
    if(find(this->logger[workloadId]["volumes"].begin(),this->logger[workloadId]["volumes"].end(),newVolume) == this->logger[workloadId]["volumes"].end())
        this->logger[workloadId]["volumes"].push_back(newVolume);

    this->logger[workloadId]["step"] = step;
    this->logger[workloadId]["jobid"].push_back(workloadId);
    this->logger[workloadId]["rackid"].push_back(workloads[workloadId].allocation.allocatedRack->rackId);
    if(layout.loadFactor(workloads, pendingToSchedule,runningWorkloads) > this->logger[workloadId]["loadFactor"])
        this->logger[workloadId]["loadFactor"] = layout.loadFactor(workloads, pendingToSchedule,runningWorkloads);
    this->logger[workloadId]["deadline"].push_back(workloads[workloadId].deadline);
    this->logger[workloadId]["arrival"].push_back(workloads[workloadId].arrival);
    this->logger[workloadId]["scheduled"].push_back(workloads[workloadId].scheduled);
}
