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
                int wlTTL = wload->executionTime + step;
                int compositionTTL = it->compositionTTL(workloads, i, step);
                int compositionTotalBw = it->compositions[i].composedNvme.getTotalBandwidth();
                int compositionAvailBw = it->compositions[i].composedNvme.getAvailableBandwidth();
                int estimateTTL = this->model.timeDistortion(
                        compositionAvailBw,wload->executionTime,wload->performanceMultiplier,
                        wload->baseBandwidth, wload->limitPeakBandwidth
                        ) + step;

                if ((deadline == -1 || deadline >= estimateTTL) &&
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
//        cout << it->rack->compositions[it->composition].assignedWorkloads.size() << endl;
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
        double bwExtra = (deadline == -1) ? 0 : log((deadline-step)/wload->executionTime) / log(wload->performanceMultiplier);
        if(deadline != -1 && bwExtra >= 1 ) {
            if(wload->baseBandwidth*bwExtra > wload->limitPeakBandwidth) {
                int newTime = pow(wload->performanceMultiplier,wload->limitPeakBandwidth/wload->baseBandwidth)*wload->executionTime;
                if((step+newTime) <= deadline ) {
                    bandwidth = wload->limitPeakBandwidth;
//                    cerr << bandwidth << endl;
//                    cerr << wloadIt << " " << newTime << " " << deadline << " " << step + newTime << " " << wload->arrival << " " << step << endl;
                }
            } else {
                int newTime = pow(wload->performanceMultiplier,bwExtra/wload->baseBandwidth)*wload->executionTime;
                if((step+newTime) <= deadline ) {
                    bandwidth = wload->baseBandwidth*bwExtra;
//                    cerr << bandwidth << endl;
//                    cerr << wloadIt << " " << newTime << " " << deadline << " " << step + newTime << " " << wload->arrival << " " << step << endl;
                }
            }
        }
        vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, bandwidth, capacity );
        if(bandwidth>2000 && selection.empty()) cerr << "empty selection" << endl;
        int selectionBw = 0;
        for(auto it2 = selection.begin(); it2!=selection.end(); ++it2) {
            selectionBw = it->resources[*it2].getTotalBandwidth();
        }
        if(!selection.empty()) {
//        if (!selection.empty() && (deadline == -1 || deadline >= (wload->executionTime+step))) {
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
        wload->nvmeBandwidth = (composedBandwidth > wload->limitPeakBandwidth) ? wload->limitPeakBandwidth : composedBandwidth;
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(composedBandwidth-wload->nvmeBandwidth);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(composedCapacity-capacity);
        scheduledRack->compositions[freeComposition].volumes = element.selection;
        scheduledRack->compositions[freeComposition].workloadsUsing = 1;
        scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(wloadIt);
        wload->executionTime = this->model.timeDistortion(composedBandwidth,
                wload->executionTime, wload->performanceMultiplier, wload->baseBandwidth, wload->limitPeakBandwidth);
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
    int totalExecutionTime = 0;
    bool scheduled = false;
    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(auto it = wloads.begin(); it!= wloads.end(); ++it) {
        capacity+=workloads[*it].nvmeCapacity;
        bandwidth+=workloads[*it].nvmeBandwidth;
        totalExecutionTime+=workloads[*it].executionTime;
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
        int resCap = 0;
        bool found = false;
        vector<int> tempSelection;
        for(auto it2 = sortBw.begin(); !found && it2!=sortBw.end(); ++it2) {
            resBw+=it->resources[*it2].getTotalBandwidth();
            resCap+=it->resources[*it2].getTotalCapacity();
            tempSelection.push_back(*it2);
            if((resBw >= bandwidth && resCap >= capacity) && (minBandwidth == -1 || minBandwidth > resBw)) {
                minBandwidth = resBw;
                found = true;
                rackFitness element = {(it->numFreeResources - (int) tempSelection.size()), it->inUse(),
                                       tempSelection, &(*it)
                };
                insertRackSorted(fittingRacks, element);
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
            wload->executionTime = this->model.timeDistortion(composedBandwidth,
                    wload->executionTime,
                    wload->performanceMultiplier,
                    wload->baseBandwidth,
                    wload->limitPeakBandwidth);
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
        }
    }

    return scheduled;
}