#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
#include "placementPolicy.hpp"
#include "degradationModel.hpp"
using namespace std;

PlacementPolicy::PlacementPolicy(DegradationModel degradationModel) {
    this->model = degradationModel;
}

void PlacementPolicy::freeResources(vector<workload>& workloads, int wloadIt) {
    workload* wload = &workloads[wloadIt];

    wload->allocation.allocatedRack->compositions
    [wload->allocation.composition].composedNvme
            .setAvailableBandwidth(wload->allocation.allocatedRack->compositions
                                   [wload->allocation.composition].composedNvme
                                           .getAvailableBandwidth()+wload->nvmeBandwidth);

    wload->allocation.allocatedRack->compositions
    [wload->allocation.composition].composedNvme
            .setAvailableCapacity(wload->allocation.allocatedRack->compositions
                                  [wload->allocation.composition].composedNvme
                                          .getAvailableCapacity()+wload->nvmeCapacity);

    //Remove Workload from assigned compositions Wloads
    bool found = false;
    for(auto i = wload->allocation.allocatedRack->
            compositions[wload->allocation.composition].assignedWorkloads.begin();
        !found && i!=wload->allocation.allocatedRack->
                compositions[wload->allocation.composition].assignedWorkloads.end();
        ++i) {
        workload ptWload = workloads[*i];
        if(ptWload.wlId == wload->wlId) {
            found = true;
            wload->allocation.allocatedRack->compositions[wload->allocation.composition].assignedWorkloads.erase(i);
        }
    }

    //if no more workloads in comopsition, free composition
    if((--wload->allocation.allocatedRack->compositions[wload->allocation.composition].workloadsUsing)==0) {
        wload->allocation.allocatedRack->freeComposition(wload->allocation.allocatedRack, wload->allocation.composition);
    } else {
        //Update other workloads times
        this->updateRackWorkloadsTime(workloads, wload->allocation.allocatedRack->compositions[wload->allocation.composition]);
    }
    wload->allocation = {};
}

vector<int> PlacementPolicy::MinSetHeuristic(vector<NvmeResource>& resources, vector<int> freeResources, int bw, int cap, int minDevices) {
    assert(minDevices > 0);
    vector<int> sortBw;
    vector<int> sortCap;
    int totalBandwidth = 0;
    int totalCapacity = 0;
    for(int i = 0; i<freeResources.size(); ++i) {
        if(freeResources[i]) {
            this->insertSortedBandwidth(resources, sortBw, i);
            this->insertSortedCapacity(resources, sortCap, i);
            totalBandwidth += resources[i].getTotalBandwidth();
            totalCapacity += resources[i].getTotalCapacity();
        }
    }

    if(totalBandwidth < bw || totalCapacity < cap || freeResources.size()<minDevices) {
        return vector<int>();
    }

    int bBw = 0;
    int bCap = 0;
    vector<int> candidatesBw;
    for(auto it = sortBw.begin(); bBw < bw && it!=sortBw.end() && minDevices > candidatesBw.size(); ++it) {
        candidatesBw.push_back(*it);
        bBw += resources[*it].getTotalBandwidth();
        bCap += resources[*it].getTotalCapacity();
    }

    if(bCap >= cap && minDevices <= candidatesBw.size())
        return candidatesBw;

    int cBw = 0;
    int cCap = 0;
    vector<int> candidatesCap;
    for(auto it = sortCap.begin(); cCap < cap && it!=sortCap.end() && minDevices > candidatesCap.size(); ++it) {
        candidatesCap.push_back(*it);
        cBw += resources[*it].getTotalBandwidth();
        cCap += resources[*it].getTotalCapacity();
    }

    if(cBw >= bw && minDevices <= candidatesCap.size())
        return candidatesCap;

    if(candidatesBw.size() >= candidatesCap.size()) {
        for(auto it = sortCap.begin(); bCap < cap && it!=sortCap.end() && minDevices > candidatesBw.size(); ++it) {
            if(find(candidatesBw.begin(),candidatesBw.end(),*it) == candidatesBw.end()) {
                bCap += resources[*it].getTotalCapacity();
                candidatesBw.push_back(*it);
            }
        }
        if(candidatesBw.size() < minDevices)
            return vector<int>();
        else
            return candidatesBw;
    } else {
        for(auto it = sortBw.begin(); cBw < bw && it!=sortBw.end() && minDevices > candidatesCap.size(); ++it) {
            if(find(candidatesCap.begin(),candidatesCap.end(),*it) == candidatesCap.end()) {
                cBw += resources[*it].getTotalBandwidth();
                candidatesCap.push_back(*it);
            }
        }

        if(candidatesCap.size() < minDevices)
            return vector<int>();
        else
            return candidatesCap;
    }
}

void PlacementPolicy::insertSortedBandwidth(vector<NvmeResource>& resources, vector<int>& vect, int element) {
    NvmeResource resource = resources[element];
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        NvmeResource current = resources[*it];
        if(resource.getTotalBandwidth() > current.getTotalBandwidth()) {
            vect.insert(it,element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

void PlacementPolicy::insertSortedCapacity(vector<NvmeResource>& resources, vector<int>& vect, int element) {
    NvmeResource resource = resources[element];
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        NvmeResource current = resources[*it];
        if(resource.getTotalCapacity() > current.getTotalCapacity()) {
            vect.insert(it,element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

void PlacementPolicy::updateRackWorkloads(vector <workload>& workloads, int wloadIt, Rack* rack, raid& composition, int compositionId) {
    workload* wload = &workloads[wloadIt];
    composition.composedNvme.setAvailableCapacity(
            (composition.composedNvme.getAvailableCapacity()-wload->nvmeCapacity)
    );
    composition.composedNvme.setAvailableBandwidth(
            (composition.composedNvme.getAvailableBandwidth()-wload->nvmeBandwidth)
    );

    wload->allocation.composition = compositionId;
    wload->allocation.allocatedRack = rack;
    composition.workloadsUsing++;
    wload->timeLeft = this->model.timeDistortion(
            composition.volumes.size(),
            composition.workloadsUsing);
    wload->executionTime = wload->timeLeft;
    this->updateRackWorkloadsTime(workloads, composition);
    composition.assignedWorkloads.push_back(wloadIt);
}

void PlacementPolicy::updateRackWorkloadsTime(vector<workload>& workloads, raid& composition) {
    for(auto iw = composition.assignedWorkloads.begin();
        iw != composition.assignedWorkloads.end(); ++iw) {
        workload it2 = workloads[*iw];
        int newTime = this->model.timeDistortion(
                composition.volumes.size(),
                composition.workloadsUsing);
        it2.timeLeft = ((float)it2.timeLeft/it2.executionTime)*newTime;
        it2.executionTime = newTime;
    }
}