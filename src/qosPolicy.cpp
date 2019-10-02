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
        } else if(it->ttlDifference == element.ttlDifference) {
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
    for(vector<Rack>::iterator it = layout.racks.begin(); it->freeCores >= wload->cores && it!=layout.racks.end(); ++it) {
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
        it->rack->freeCores -= wload->cores;
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
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it->freeCores >= wload->cores && it!=layout.racks.end(); ++it) {
        float bwExtra = (deadline == -1) ? 0 : (
                (log((float)((float)(deadline-step)/(float)wload->executionTime)) / log((float)wload->performanceMultiplier)  ) + 1
                ) * wload->baseBandwidth;
//        if(bwExtra>wload->baseBandwidth) cerr << bwExtra << " " << deadline << " " << step << " " << wload->executionTime << " " << wload->performanceMultiplier << " " << wload->baseBandwidth << endl;
        if(deadline != -1 && bwExtra >= 1 && bwExtra >= wload->nvmeBandwidth) {
            if(bwExtra > wload->limitPeakBandwidth) {
                int newTime = pow(wload->performanceMultiplier,(wload->limitPeakBandwidth/wload->baseBandwidth)-1)*wload->executionTime;
                if((step+newTime) <= deadline ) {
                    bandwidth = wload->limitPeakBandwidth;
                }
            } else {
                int newTime = pow(wload->performanceMultiplier,bwExtra/wload->baseBandwidth-1)*wload->executionTime;
                if((step+newTime) <= deadline ) {
                    bandwidth = bwExtra;
                }
            }
        }
        vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, bandwidth, capacity );
        if(selection.empty() && bandwidth > wload->nvmeBandwidth) {
            //try new selection
            bandwidth = wload->nvmeBandwidth;
            vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, wload->nvmeBandwidth, capacity );
//            if(selection.empty() && it->resourcesUsed() < 1) cerr << wload->nvmeBandwidth << " " << wloadIt << endl;
//            if(selection.empty() && it->resourcesUsed() < 1) cerr << step << " " << it->getAvailableBandwidth() << endl;
//            if(selection.empty() && it->resourcesUsed() < 1) {
//                cerr << step << endl; layout.printRaidsInfo();
//            }
        }
        if(!selection.empty()) {
            int bwSel = 0;
            for(auto it2 = selection.begin(); it2!=selection.end(); ++it2) {
                bwSel += it->resources[*it2].getAvailableBandwidth();
            }
            int ttl = this->model.timeDistortion(bwSel,
                                             wload->executionTime,
                                             wload->performanceMultiplier,
                                             wload->baseBandwidth,
                                             wload->limitPeakBandwidth);

//        if (!selection.empty() && (deadline == -1 || deadline >= (wload->executionTime+step))) {
            rackFitness element = {ttl, it->inUse(),
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
        scheduledRack->freeCores -= wload->cores;
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
    int cores = 0;
    int totalExecutionTime = 0;
    bool scheduled = false;
    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(auto it = wloads.begin(); it!= wloads.end(); ++it) {
        capacity+=workloads[*it].nvmeCapacity;
        bandwidth+=workloads[*it].nvmeBandwidth;
        totalExecutionTime+=workloads[*it].executionTime;
        cores+=workloads[*it].cores;
    }

    for(auto it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        vector<NvmeResource> res = it->resources;
        vector<int> sortBw;
        for(int i = 0; i<it->freeResources.size(); ++i) {
            if(it->freeResources[i]) {
                this->insertSortedBandwidth(it->resources, sortBw, i);
            }
        }
        vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, bandwidth, capacity );
        if(!selection.empty()) {
            int bwSel = 0;
            for(auto it2 = selection.begin(); it2!=selection.end(); ++it2) {
                bwSel += it->resources[*it2].getAvailableBandwidth();
            }
            int ttl = 0;
            for(auto it3 = wloads.begin(); it3!=wloads.end(); ++it3) {
                workload* wload = &workloads[*it3];
//                int wlTTL = this->model.timeDistortion(bwSel,
//                                                     wload->executionTime,
//                                                     wload->performanceMultiplier,
//                                                     wload->baseBandwidth,
//                                                     wload->limitPeakBandwidth);
                if(ttl < wload->executionTime)
                    ttl = wload->executionTime;
            }

            rackFitness element = {ttl, it->inUse(),
                                   selection, &(*it)
            };
            insertRackSorted(fittingRacks, element);
        }
//        int resBw = 0;
//        int resCap = 0;
//        bool found = false;
//        vector<int> tempSelection;
//        for(auto it2 = sortBw.begin(); !found && it2!=sortBw.end(); ++it2) {
//            resBw+=it->resources[*it2].getTotalBandwidth();
//            resCap+=it->resources[*it2].getTotalCapacity();
//            tempSelection.push_back(*it2);
//            if((resBw >= bandwidth && resCap >= capacity) && (minBandwidth == -1 || minBandwidth > resBw)) {
//                minBandwidth = resBw;
//                found = true;
//                rackFitness element = {(it->numFreeResources - (int) tempSelection.size()), it->inUse(),
//                                       tempSelection, &(*it)
//                };
//                insertRackSorted(fittingRacks, element);
//            }
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

        scheduledRack->numFreeResources-=element.selection.size();
        scheduledRack->compositions[freeComposition].used = true;
        scheduledRack->compositions[freeComposition].composedNvme = NvmeResource(composedBandwidth,composedCapacity);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(composedBandwidth-bandwidth);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(composedCapacity-capacity);
        scheduledRack->compositions[freeComposition].volumes = element.selection;
        scheduledRack->compositions[freeComposition].workloadsUsing = wloads.size();
        scheduledRack->freeCores -= cores;
        for(auto it = wloads.begin(); it!=wloads.end(); ++it) {
            workload* wload = &workloads[*it];
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(*it);
//            wload->executionTime = this->model.timeDistortion(composedBandwidth,
//                    wload->executionTime,
//                    wload->performanceMultiplier,
//                    wload->baseBandwidth,
//                    wload->limitPeakBandwidth);
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
        }
    }

    return scheduled;
}