#include <iostream>
#include <vector>
#include <algorithm>
#include "math.h"
#include "qosPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

bool QoSPolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    if( !this->placeWorkloadInComposition(workloads,wloadIt,layout,step,deadline) ) {
        return this->placeWorkloadNewComposition(workloads, wloadIt, layout, step, deadline);
    } else
        return true;
}

void QoSPolicy::insertRackSorted(vector<rackFitness>& vect, rackFitness& element) {
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

void QoSPolicy::insertSorted(vector<nvmeFitness>& vect, nvmeFitness& element) {
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if(it->ttlDifference > element.ttlDifference) {
            vect.insert(it,element);
            inserted = true;
        } else if(it->ttlDifference = element.ttlDifference) {
            if (it->fitness >= element.fitness) {
                vect.insert(it, element);
                inserted = true;
            }
        }
    }
    if(!inserted)
        vect.push_back(element);
}

bool QoSPolicy::placeWorkloadInComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    vector<nvmeFitness> fittingCompositions;
    bool scheduled = false;
    for(vector<Rack>::iterator it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        int position = 0;
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used) {
                int wlTTL = wload->executionTime;
                int compositionTTL = it->compositionTTL(workloads, i, step);
                int compositionTotalBw = it->compositions[i].composedNvme.getTotalBandwidth();
                int compositionAvailBw = it->compositions[i].composedNvme.getAvailableBandwidth();
                int estimateTTL = this->model.timeDistortion(compositionTotalBw,
                                                             (compositionTotalBw - compositionAvailBw) + wload->nvmeBandwidth);
                estimateTTL+=step;

                bool valid = true;
                for(auto iw = it->compositions[i].assignedWorkloads.begin();
                    valid && iw != it->compositions[i].assignedWorkloads.end(); ++iw) {
                    workload it2 = workloads[*iw];
                    int newTime = this->model.timeDistortion(
                            compositionTotalBw,
                            (compositionTotalBw - compositionAvailBw) + wload->nvmeBandwidth);
                    it2.timeLeft = ((float)it2.timeLeft/it2.executionTime)*newTime;
                    int deadlineLeft = it2.deadline - step;
                    if(step > it2.deadline || it2.timeLeft > deadlineLeft)
                        valid = false;
                }

                if ((deadline == -1 || deadline >= estimateTTL) && valid &&
                    it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
                    it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity) {

                    nvmeFitness element = {
                            ((it->compositions[i].composedNvme.getAvailableBandwidth() - wload->nvmeBandwidth)
                             + (it->compositions[i].composedNvme.getAvailableCapacity() - wload->nvmeCapacity)),
                            estimateTTL - compositionTTL, i, &(*it)
                    };
//                    if(compositionTTL <= estimateTTL)
                    this->insertSorted(fittingCompositions, element);
                }
            }
        }
    }

    if(!fittingCompositions.empty()) {
        vector<nvmeFitness>::iterator it = fittingCompositions.begin();
        this->updateRackWorkloads(workloads,wloadIt, it->rack, it->rack->compositions[it->composition], it->composition);
        scheduled = true;
    }

    return scheduled;
}

bool QoSPolicy::placeWorkloadNewComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    bool scheduled = false;
    workload* wload = &workloads[wloadIt];
    int capacity = wload->nvmeCapacity;
    int bandwidth = wload->nvmeBandwidth;

    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, capacity, bandwidth);
        int selectionBw = 0;
        for(auto it2 = selection.begin(); it2!=selection.end(); ++it2) {
            selectionBw = it->resources[*it2].getTotalBandwidth();
        }
        if (!selection.empty() && (deadline == -1 || deadline >= this->model.timeDistortion(selectionBw,bandwidth))) {
            rackFitness element = {(it->numFreeResources - (int) selection.size()), it->inUse(),
                                   selection, &(*it)
            };
            insertRackSorted(fittingRacks, element);
        }
    }

    if(!fittingRacks.empty()) {
        rackFitness element = *fittingRacks.begin();
        scheduledRack = element.rack;
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
        scheduledRack->compositions[freeComposition].workloadsUsing = 1;
        scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(wloadIt);
        wload->executionTime = this->model.timeDistortion(composedBandwidth,bandwidth);
        wload->timeLeft = wload->executionTime;
        wload->allocation.composition = freeComposition;
        wload->allocation.allocatedRack = scheduledRack;
    }

    return scheduled;
}

bool QoSPolicy::placeWorkloadsNewComposition(vector<workload>& workloads, vector<int>& wloads, Layout& layout, int step) {
    int deadline = workloads[*(wloads.begin())].deadline;
    int minBandwidth = -1;
    int capacity = 0;
    int bandwidth = 0;
    bool scheduled = false;
    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(auto it = wloads.begin(); it!= wloads.end(); ++it) {
        capacity+=workloads[*it].nvmeCapacity;
        bandwidth+=workloads[*it].nvmeBandwidth;
    }

    for(auto it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        vector<NvmeResource> res = it->resources;
        vector<int> sortBw;
        for(int i = 0; i<it->freeResources.size(); ++i) {
            if(it->freeResources[i]) {
                this->insertSortedBandwidth(it->resources, sortBw, i);
            }
        }
        int resBw = 0;
        bool found = false;
        vector<int> tempSelection;
        for(auto it2 = sortBw.begin(); !found && it2!=sortBw.end(); ++it2) {
            resBw+=it->resources[*it2].getTotalBandwidth();
            tempSelection.push_back(*it2);
            if(this->model.timeDistortion(resBw,bandwidth) <= (deadline - step)) {
                if(bandwidth <= resBw && (minBandwidth == -1 || minBandwidth  > resBw)) {
                    minBandwidth = resBw;
                    found = true;
                    rackFitness element = {(it->numFreeResources - (int) tempSelection.size()), it->inUse(),
                                           tempSelection, &(*it)
                    };
                    insertRackSorted(fittingRacks, element);
                }
            }
        }
    }


    if(!fittingRacks.empty()) {
        rackFitness element = *fittingRacks.begin();
        scheduledRack = element.rack;
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
        for(auto it = wloads.begin(); it!=wloads.end(); ++it) {
            workload* wload = &workloads[*it];
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(*it);
            wload->executionTime = this->model.timeDistortion(composedBandwidth, bandwidth);
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
        }
    }

    return scheduled;
}