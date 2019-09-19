#include <iostream>
#include <vector>
#include <algorithm>
#include "math.h"
#include "minFragPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

inline int maxConcurrency(int numVolumes, double loadFactor) {
//    if(loadFactor <= 0.6)
        return 1+2*numVolumes;
//    else
//        return 2+2*numVolumes;
}

bool MinFragPolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    bool scheduled = this->placeWorkloadInComposition(workloads, wloadIt, layout, step, deadline);
    if(!scheduled)
        return this->placeWorkloadNewComposition(workloads, wloadIt, layout, step, deadline);

    return scheduled;
}

void MinFragPolicy::insertRackSorted(vector<rackFitness>& vect, rackFitness& element) {
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

void MinFragPolicy::insertSorted(vector<nvmeFitness>& vect, nvmeFitness& element) {
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if(it->fitness < element.fitness) {
            vect.insert(it,element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}
//
//void MinFragPolicy::insertSorted(vector<nvmeFitness>& vect, nvmeFitness& element) {
//    bool inserted = false;
//    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
//        if(it->ttlDifference > element.ttlDifference) {
//            vect.insert(it,element);
//            inserted = true;
//        } else if(it->ttlDifference = element.ttlDifference) {
//            if (it->fitness >= element.fitness) {
//                vect.insert(it, element);
//                inserted = true;
//            }
//        }
//    }
//    if(!inserted)
//        vect.push_back(element);
//}

bool MinFragPolicy::placeWorkloadInComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
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
//                int estimateTTL = this->model.timeDistortion(compositionTotalBw,
//                                                             (compositionTotalBw - compositionAvailBw) + wload->nvmeBandwidth);
                int estimateTTL = wlTTL + step;

                if ((deadline==-1 || estimateTTL <= deadline) && (compositionAvailBw >= wload->nvmeBandwidth &&
                     it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity)) {

                    nvmeFitness element = {
                            ((compositionAvailBw - wload->nvmeBandwidth)
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
        this->updateRackWorkloads(workloads, wloadIt, it->rack, it->rack->compositions[it->composition],
                                  it->composition);
        scheduled = true;
    }

    return scheduled;
}

bool MinFragPolicy::placeWorkloadNewComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    bool scheduled = false;
    workload* wload = &workloads[wloadIt];
    int capacity = wload->nvmeCapacity;
    int bandwidth = wload->nvmeBandwidth;

    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        vector<int> selection = this->MinFragHeuristic(it->resources, it->freeResources, bandwidth, capacity);
        if(!selection.empty()) {
            rackFitness element = {((int) selection.size()), it->inUse(),
                                   selection, &(*it)
            };
            insertRackSorted(fittingRacks, element);
        }
//        } else if(wloadIt == 7) {
//            cerr << "yES" << " " << layout.resourcesUsed() << endl;
//            layout.printRaidsInfo();
//        }
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

        int selectionBw = 0;
        for(auto it = element.selection.begin(); it!=element.selection.end(); ++it) {
            selectionBw += scheduledRack->resources[*it].getTotalBandwidth();
        }

        scheduledRack->numFreeResources-=element.selection.size();
        scheduledRack->compositions[freeComposition].used = true;
        scheduledRack->compositions[freeComposition].used = true;
        scheduledRack->compositions[freeComposition].composedNvme = NvmeResource(composedBandwidth,composedCapacity);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(composedBandwidth-bandwidth);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(composedCapacity-capacity);
        scheduledRack->compositions[freeComposition].volumes = element.selection;
        scheduledRack->compositions[freeComposition].workloadsUsing = 1;
        scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(wloadIt);
        wload->executionTime = this->model.timeDistortion(composedBandwidth,
              wload->executionTime,
              wload->performanceMultiplier,
              wload->baseBandwidth,
              wload->limitPeakBandwidth
        );
//        wload->executionTime = this->model.timeDistortion(selectionBw,bandwidth);
        wload->timeLeft = wload->executionTime;
        wload->allocation.composition = freeComposition;
        wload->allocation.allocatedRack = scheduledRack;
    }

    return scheduled;
}

bool MinFragPolicy::placeWorkloadsNewComposition(vector<workload>& workloads, vector<int>& wloads, Layout& layout, int step) {
    int capacity = 0;
    int bandwidth = 0;
    for(auto it = wloads.begin(); it!= wloads.end(); ++it) {
        capacity+=workloads[*it].nvmeCapacity;
        bandwidth+=workloads[*it].nvmeBandwidth;
    }

    bool scheduled = false;
    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        vector<int> selection = this->MinFragHeuristic(it->resources, it->freeResources, bandwidth, capacity);
        if (!selection.empty()) {
            rackFitness element = {((int) selection.size()), it->inUse(),
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
        scheduledRack->compositions[freeComposition].workloadsUsing = wloads.size();
        for(auto it = wloads.begin(); it!=wloads.end(); ++it) {
            workload* wload = &workloads[*it];
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(*it);
            wload->executionTime = this->model.timeDistortion(composedBandwidth,
                    wload->executionTime,
                    wload->performanceMultiplier,
                    wload->baseBandwidth,
                    wload->limitPeakBandwidth
            );
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
        }
    }

    return scheduled;
}