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

    assert(wload->allocation.composition != -1 || (wload->nvmeBandwidth == 0 && wload->nvmeCapacity == 0));
    if(wload->allocation.composition != -1) {
        wload->allocation.allocatedRack->compositions
        [wload->allocation.composition].composedNvme
                .setAvailableBandwidth(wload->allocation.allocatedRack->compositions
                                       [wload->allocation.composition].composedNvme
                                               .getAvailableBandwidth() + wload->nvmeBandwidth);

        wload->allocation.allocatedRack->compositions
        [wload->allocation.composition].composedNvme
                .setAvailableCapacity(wload->allocation.allocatedRack->compositions
                                      [wload->allocation.composition].composedNvme
                                              .getAvailableCapacity() + wload->nvmeCapacity);

        //Remove Workload from assigned compositions Wloads
        bool found = false;
        for (auto i = wload->allocation.allocatedRack->
                compositions[wload->allocation.composition].assignedWorkloads.begin();
             !found && i != wload->allocation.allocatedRack->
                     compositions[wload->allocation.composition].assignedWorkloads.end();
             ++i) {
            workload ptWload = workloads[*i];
            if (ptWload.wlId == wload->wlId) {
                found = true;
                wload->allocation.allocatedRack->compositions[wload->allocation.composition].assignedWorkloads.erase(i);
            }
        }

        //if no more workloads in comopsition, free composition
        if ((--wload->allocation.allocatedRack->compositions[wload->allocation.composition].workloadsUsing) == 0) {
            wload->allocation.allocatedRack->freeComposition(wload->allocation.allocatedRack,
                                                             wload->allocation.composition);
        } else {
            //Update other workloads times
            this->updateRackWorkloadsTime(workloads,
                                          wload->allocation.allocatedRack->compositions[wload->allocation.composition]);
        }
    }
    wload->allocation.coresAllocatedRack->freeCores += wload->cores;
//    wload->allocation.allocatedRack->freeCores += wload->cores;
    wload->allocation = allocatedResources();
}

vector<int> PlacementPolicy::MinSetHeuristic(vector<NvmeResource>& resources, vector<int> freeResources, int bw, int cap) {
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

    if(totalBandwidth < bw || totalCapacity < cap) {
        return vector<int>();
    }

    int bBw = 0;
    int bCap = 0;
    vector<int> candidatesBw;
    for(auto it = sortBw.begin(); bBw < bw && it!=sortBw.end(); ++it) {
        candidatesBw.push_back(*it);
        bBw += resources[*it].getTotalBandwidth();
        bCap += resources[*it].getTotalCapacity();
    }

    if(bCap >= cap)
        return candidatesBw;

    int cBw = 0;
    int cCap = 0;
    vector<int> candidatesCap;
    for(auto it = sortCap.begin(); cCap < cap && it!=sortCap.end(); ++it) {
        candidatesCap.push_back(*it);
        cBw += resources[*it].getTotalBandwidth();
        cCap += resources[*it].getTotalCapacity();
    }

    if(cBw >= bw)
        return candidatesCap;

    if(candidatesBw.size() >= candidatesCap.size()) {
        for(auto it = sortCap.begin(); (bBw < bw || bCap < cap) && it!=sortCap.end(); ++it) {
            if(find(candidatesBw.begin(),candidatesBw.end(),*it) == candidatesBw.end()) {
                bCap += resources[*it].getTotalCapacity();
                bBw += resources[*it].getTotalBandwidth();
                candidatesBw.push_back(*it);
            }
        }

        return candidatesBw;
    } else {
        for(auto it = sortBw.begin(); (cCap < cap || cBw < bw) && it!=sortBw.end(); ++it) {
            if(find(candidatesCap.begin(),candidatesCap.end(),*it) == candidatesCap.end()) {
                cBw += resources[*it].getTotalBandwidth();
                cCap += resources[*it].getTotalCapacity();
                candidatesCap.push_back(*it);
            }
        }

        return candidatesCap;
    }
}

vector<int> PlacementPolicy::MinFragHeuristic(vector<NvmeResource>& resources, vector<int> freeResources, int bw, int cap) {
    vector<int> sortBw;
    vector<int> sortCap;
    int totalBandwidth = 0;
    int totalCapacity = 0;
    int resourceBw = -1;
    int resourceCap = -1;
    for(int i = 0; i<freeResources.size(); ++i) {
        if(freeResources[i]) {
            this->insertSortedBandwidth(resources, sortBw, i);
            this->insertSortedCapacity(resources, sortCap, i);
            totalBandwidth += resources[i].getTotalBandwidth();
            totalCapacity += resources[i].getTotalCapacity();
            if(resourceBw == -1 && resources[i].getTotalBandwidth() >= bw) {
                resourceBw = i;
            }
            if(resourceCap == -1 && resources[i].getTotalCapacity() >= cap) {
                resourceCap = i;
            }
        }
    }

    if(totalBandwidth < bw || totalCapacity < cap) {
        return vector<int>();
    }

    int bBw = 0;
    int bCap = 0;
    vector<int> candidatesBw;
    for(auto it = sortBw.begin(); bBw < bw && it!=sortBw.end(); ++it) {
        if(resourceBw == -1 || (resourceBw != -1 && resources[*it].getTotalBandwidth() >= bw)) {
            candidatesBw.push_back(*it);
            bBw += resources[*it].getTotalBandwidth();
            bCap += resources[*it].getTotalCapacity();
        }
    }

    if(bCap >= cap && bBw >= bw)
        return candidatesBw;

    int cBw = 0;
    int cCap = 0;
    vector<int> candidatesCap;
    for(auto it = sortCap.begin(); cCap < cap && it!=sortCap.end(); ++it) {
        if(resourceCap == -1 || (resourceCap != -1 && resources[*it].getTotalCapacity() >= cap)) {
            candidatesCap.push_back(*it);
            cBw += resources[*it].getTotalBandwidth();
            cCap += resources[*it].getTotalCapacity();
        }
    }

    if(cBw >= bw && cCap >= cap)
        return candidatesCap;

    if(candidatesBw.size() >= candidatesCap.size()) {
        for(auto it = sortCap.begin(); (bBw < bw || bCap < cap) && it!=sortCap.end(); ++it) {
            if(find(candidatesBw.begin(),candidatesBw.end(),*it) == candidatesBw.end()) {
                bCap += resources[*it].getTotalCapacity();
                bBw += resources[*it].getTotalBandwidth();
                candidatesBw.push_back(*it);
            }
        }

        return candidatesBw;
    } else {
        for(auto it = sortBw.begin(); (cCap < cap || cBw < bw) && it!=sortBw.end(); ++it) {
            if(find(candidatesCap.begin(),candidatesCap.end(),*it) == candidatesCap.end()) {
                cBw += resources[*it].getTotalBandwidth();
                cCap += resources[*it].getTotalCapacity();
                candidatesCap.push_back(*it);
            }
        }

        return candidatesCap;
    }
}

void PlacementPolicy::insertSortedBandwidth(vector<NvmeResource>& resources, vector<int>& vect, int element) {
    NvmeResource resource = resources[element];
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        NvmeResource current = resources[*it];
        if(resource.getTotalBandwidth() < current.getTotalBandwidth()) {
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
        if(resource.getTotalCapacity() < current.getTotalCapacity()) {
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
            (composition.composedNvme.getAvailableCapacity() - wload->nvmeCapacity));
    if(wload->wlName != "smufin")
        composition.composedNvme.setAvailableBandwidth(
                (composition.composedNvme.getAvailableBandwidth()-wload->nvmeBandwidth)
        );

    wload->allocation.composition = compositionId;
    wload->allocation.allocatedRack = rack;
    composition.workloadsUsing++;
    wload->timeLeft = wload->executionTime;

    if(wload->nvmeBandwidth > 0) {
        wload->timeLeft = this->model.timeDistortion(composition,workloads[wloadIt]);
    }

//    wload->nvmeBandwidth = (composition.composedNvme.getAvailableBandwidth() > wload->limitPeakBandwidth) ? wload->limitPeakBandwidth : composition.composedNvme.getAvailableBandwidth();
    wload->executionTime = wload->timeLeft;
//    this->updateRackWorkloadsTime(workloads, composition);
    composition.assignedWorkloads.push_back(wloadIt);
}

void PlacementPolicy::updateRackWorkloadsTime(vector<workload>& workloads, raid& composition) {
    for(auto iw = composition.assignedWorkloads.begin();
        iw != composition.assignedWorkloads.end(); ++iw) {
        workload it2 = workloads[*iw];
        if(it2.nvmeBandwidth > 0) {
            int newTime = this->model.timeDistortion(composition,it2);
            it2.timeLeft = ((float) it2.timeLeft / it2.executionTime) * newTime;
            it2.executionTime = newTime;
        }
    }
}