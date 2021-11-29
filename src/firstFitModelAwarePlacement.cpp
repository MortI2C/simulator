#include <iostream>
#include <vector>
#include <algorithm>
#include "assert.h"
#include "math.h"
#include "firstFitModelAwarePlacement.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;


void FirstFitModelAwarePolicy::insertRackSorted(vector<rackFitness>& vect, rackFitness& element) {
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if(!it->inUse && element.inUse) {
            vect.insert(it,element);
            inserted = true;
        } else if (it->fitness >= element.fitness) {
            vect.insert(it, element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

bool FirstFitModelAwarePolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    if(wload->nvmeBandwidth == 0 && wload->nvmeCapacity == 0) {
        return this->placeExecOnlyWorkload(workloads, wloadIt, layout, step, deadline);
    }

    workloads[wloadIt].allocationAttempts++;
    bool scheduled = this->placeWorkloadInComposition(workloads, wloadIt, layout, step, deadline);
    if(!scheduled) {
        workloads[wloadIt].allocationAttempts++;
        return this->placeWorkloadNewComposition(workloads, wloadIt, layout, step, deadline);
    }

    return scheduled;
}

Rack* FirstFitModelAwarePolicy::allocateCoresOnly(vector<workload>& workloads, int wloadIt, Layout& layout) {
    workload* wload = &workloads[wloadIt];
    bool scheduled = false;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        if(it->freeCores >= wload->cores)
        {
           Rack* element = &(*it);
           return element;
        }
    }

    return nullptr;
}

Rack* FirstFitModelAwarePolicy::allocateWorkloadsCoresOnly(vector<workload>& workloads, vector<int>& wloads, Layout& layout) {
    int cores = 0;
    for(auto it = wloads.begin(); it!= wloads.end(); ++it) {
        cores+=workloads[*it].cores;
    }

    bool scheduled = false;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        if(it->freeCores >= cores)
        {
            Rack* element = &(*it);
            return element;
        }
    }

    return nullptr;
}

bool FirstFitModelAwarePolicy::placeExecOnlyWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    Rack* scheduledRack = this->allocateCoresOnly(workloads, wloadIt, layout);
    if(scheduledRack != nullptr) {
        scheduledRack->freeCores -= wload->cores;
        wload->timeLeft = wload->executionTime;
        wload->allocation.coresAllocatedRack = scheduledRack;
        wload->placementPolicy = "minfrag";
        return true;
    } else
        return false;
}

bool FirstFitModelAwarePolicy::placeWorkloadInComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    vector<nvmeFitness> fittingCompositions;
    bool scheduled = false;
    for(vector<Rack>::iterator it = layout.racks.begin();  it!=layout.racks.end() && !scheduled; ++it) {
        int position = 0;
        if(it->resources.begin()->getTotalCapacity()>1 && (it->cores==0 || it->freeCores >= wload->cores)) {
            for (int i = 0; !scheduled && i < it->compositions.size(); ++i) {
                if (it->compositions[i].used && it->compositions[i].coresRack->freeCores>=wload->cores &&
                    it->possibleToColocate(workloads, wloadIt, i, step, this->model)) {
                    int wlTTL = wload->executionTime + step;
                    int compositionTTL = it->compositionTTL(workloads, i, step);
                    int compositionTotalBw = it->compositions[i].composedNvme.getTotalBandwidth();
                    int compositionAvailBw = it->compositions[i].composedNvme.getAvailableBandwidth();

                    if ((wload->wlName == "smufin" && it->compositions[i].workloadsUsing < 7) ||
                        (wload->wlName != "smufin" &&
                         it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
                         it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity)) {
//                if (it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
//                    it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity) {

                        nvmeFitness element = {
                                ((it->compositions[i].composedNvme.getAvailableBandwidth() - wload->nvmeBandwidth)
                                 + (it->compositions[i].composedNvme.getAvailableCapacity() - wload->nvmeCapacity)),
                                wlTTL - compositionTTL, i, &(*it)
                        };
                        fittingCompositions.push_back(element);

                        //CHECK IF DISAGG SCENARIO OR NOT
                        Rack *coresRack = it->compositions[i].coresRack;

                        coresRack->freeCores -= wload->cores;
                        this->updateRackWorkloads(workloads, wloadIt,
                                element.rack,
                                element.rack->compositions[element.composition],
                                element.composition);
                        scheduled = true;
                        wload->allocation.coresAllocatedRack = coresRack;
                        wload->placementPolicy = "minfrag";
                        assert(wload->allocation.coresAllocatedRack == it->compositions[i].coresRack);
                    }
                } else if (it->compositions[i].used && it->compositions[i].coresRack->freeCores<wload->cores &&
                           it->possibleToColocate(workloads, wloadIt, i, step, this->model)) {
                    if ((wload->wlName == "smufin" && it->compositions[i].workloadsUsing < 7) ||
                        (wload->wlName != "smufin" &&
                         it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
                         it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity))
                        wload->failToAllocateDueCores++;
                }
            }
        }
    }

    return scheduled;
}

bool FirstFitModelAwarePolicy::placeWorkloadNewComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    bool scheduled = false;
    workload* wload = &workloads[wloadIt];
    int capacity = wload->nvmeCapacity;
    int bandwidth = wload->nvmeBandwidth;

    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin();  it!=layout.racks.end() &&
      !scheduled && fittingRacks.empty(); ++it) {
        if(it->cores>0 && it->resources.begin()->getTotalCapacity()>1 && it->freeCores < wload->cores) {
            if(it->getAvailableCapacity() >= wload->nvmeCapacity && it->getAvailableBandwidth() >= wload->nvmeBandwidth)
                wload->failToAllocateDueCores++;
        } else if(it->resources.begin()->getTotalCapacity()>1 && (it->cores==0 || it->freeCores >= wload->cores)) {
            vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, bandwidth, capacity);
            int selectionBw = 0;
            for (auto it2 = selection.begin(); it2 != selection.end(); ++it2) {
                selectionBw = it->resources[*it2].getTotalBandwidth();
            }
            if (!selection.empty()) {
                if(it->cores==0 || it->freeCores >= wload->cores) {
                    rackFitness element = {(it->numFreeResources - (int) selection.size()), it->inUse(),
                                           selection, &(*it)
                    };
                    fittingRacks.push_back(element);
                }
            }
        }
    }

    Rack* coresRack = this->allocateCoresOnly(workloads, wloadIt, layout);
    if(!fittingRacks.empty() && coresRack==nullptr)
        wload->failToAllocateDueCores++;

    if(!fittingRacks.empty() && coresRack != nullptr) {
        rackFitness element = *fittingRacks.begin();
        scheduledRack = element.rack;
        if(scheduledRack->cores > 0)
            coresRack = scheduledRack;

        scheduled = true;
        int freeComposition = -1;
        for(int i = 0; freeComposition == -1 && i<scheduledRack->compositions.size(); ++i) {
            if(!scheduledRack->compositions[i].used)
                freeComposition = i;
        }

        int composedBandwidth = 0;
        int composedCapacity = 0;
        for(int i = 0; i<element.selection.size(); ++i) {
            scheduledRack->freeResources[element.selection[i]] = 0;
            composedBandwidth += scheduledRack->resources[element.selection[i]].getTotalBandwidth();
            composedCapacity += scheduledRack->resources[element.selection[i]].getTotalCapacity();
        }

        scheduledRack->numFreeResources-=element.selection.size();
        scheduledRack->compositions[freeComposition].used = true;
//        wload->nvmeBandwidth = (composedBandwidth > wload->limitPeakBandwidth) ? wload->limitPeakBandwidth : composedBandwidth;
        scheduledRack->compositions[freeComposition].composedNvme = NvmeResource(composedBandwidth,composedCapacity);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(composedBandwidth-wload->nvmeBandwidth);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(composedCapacity-capacity);
        scheduledRack->compositions[freeComposition].volumes = element.selection;
        scheduledRack->compositions[freeComposition].workloadsUsing = 1;
        scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(wloadIt);
        scheduledRack->compositions[freeComposition].coresRack = coresRack;
        coresRack->freeCores -= wload->cores;
        wload->timeLeft = this->model.timeDistortion(scheduledRack->compositions[freeComposition],
                *wload);
        wload->allocation.composition = freeComposition;
        wload->allocation.allocatedRack = scheduledRack;
        wload->allocation.coresAllocatedRack = coresRack;
        wload->placementPolicy = "minfrag";
    }

    return scheduled;
}

bool FirstFitModelAwarePolicy::placeWorkloadsNewComposition(vector<workload>& workloads, vector<int>& wloads, Layout& layout, int step) {
    int deadline = workloads[*(wloads.begin())].deadline;
    int minBandwidth = -1;
    int capacity = 0;
    int bandwidth = 0;
    bool scheduled = false;
    int cores = 0;
    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(auto it = wloads.begin(); it!= wloads.end(); ++it) {
        capacity+=workloads[*it].nvmeCapacity;
        bandwidth+=workloads[*it].nvmeBandwidth;
        cores+=workloads[*it].cores;
    }

    bool found = false;
    for(auto it = layout.racks.begin(); !found && it!=layout.racks.end(); ++it) {
        if(it->resources.begin()->getTotalCapacity()>1) {
            vector <NvmeResource> res = it->resources;
            vector<int> sortBw;
            for (int i = 0; i < it->freeResources.size(); ++i) {
                if (it->freeResources[i]) {
                    this->insertSortedBandwidth(it->resources, sortBw, i);
                }
            }
            int resBw = 0;
            int resCap = 0;
            vector<int> tempSelection;
            for (auto it2 = sortBw.begin(); !found && it2 != sortBw.end(); ++it2) {
                resBw += it->resources[*it2].getTotalBandwidth();
                resCap += it->resources[*it2].getTotalCapacity();
                tempSelection.push_back(*it2);
                if (resBw >= bandwidth && resCap >= capacity) {
                    if(it->cores==0 || it->freeCores >= cores) {
                        found = true;
                        rackFitness element = {(it->numFreeResources - (int) tempSelection.size()), it->inUse(),
                                               tempSelection, &(*it)
                        };
                        insertRackSorted(fittingRacks, element);
                    }
                }
            }
        }
    }

    Rack* coresRack = this->allocateWorkloadsCoresOnly(workloads, wloads, layout);
    if(!fittingRacks.empty() && coresRack != nullptr) {
        rackFitness element = *fittingRacks.begin();
        scheduledRack = element.rack;
        if(scheduledRack->cores>0)
            coresRack = scheduledRack;

        scheduled = true;
        int freeComposition = -1;
        for(int i = 0; freeComposition == -1 && i<scheduledRack->compositions.size(); ++i) {
            if(!scheduledRack->compositions[i].used)
                freeComposition = i;
        }

        int composedBandwidth = 0;
        int composedCapacity = 0;
        for(int i = 0; i<element.selection.size(); ++i) {
            scheduledRack->freeResources[element.selection[i]] = 0;
            composedBandwidth += scheduledRack->resources[element.selection[i]].getTotalBandwidth();
            composedCapacity += scheduledRack->resources[element.selection[i]].getTotalCapacity();
        }

        scheduledRack->numFreeResources-=element.selection.size();
        scheduledRack->compositions[freeComposition].used = true;
        scheduledRack->compositions[freeComposition].composedNvme = NvmeResource(composedBandwidth,composedCapacity);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(composedBandwidth-bandwidth);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(composedCapacity-capacity);
        scheduledRack->compositions[freeComposition].volumes = element.selection;
        scheduledRack->compositions[freeComposition].workloadsUsing = wloads.size();
        scheduledRack->compositions[freeComposition].coresRack = coresRack;
        coresRack->freeCores -= cores;
        for(auto it = wloads.begin(); it!=wloads.end(); ++it) {
            workload* wload = &workloads[*it];
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(*it);
            wload->executionTime = this->model.timeDistortion(scheduledRack->compositions[freeComposition],*wload);
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
            wload->allocation.coresAllocatedRack = coresRack;
            wload->placementPolicy = "minfrag";
        }
    }

    return scheduled;
}