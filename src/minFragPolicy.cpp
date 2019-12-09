#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>
#include "math.h"
#include "minFragPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

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

bool MinFragPolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    if(wload->nvmeBandwidth == 0 && wload->nvmeCapacity == 0) {
        return this->placeExecOnlyWorkload(workloads, wloadIt, layout, step, deadline);
    }

    bool scheduled = this->placeWorkloadInComposition(workloads, wloadIt, layout, step, deadline);
    if(!scheduled)
        return this->placeWorkloadNewComposition(workloads, wloadIt, layout, step, deadline);

    return scheduled;
}

Rack* MinFragPolicy::allocateCoresOnly(vector<workload>& workloads, int wloadIt, Layout& layout) {
    workload* wload = &workloads[wloadIt];
    bool scheduled = false;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        if(it->freeCores >= wload->cores)
        {
            int percFreecores = (it->freeCores/it->cores)*100;
            int alpha = (percFreecores == 0) ? 0 : (((it->getTotalBandwidthUsed()/it->totalBandwidth)*100
                                                     +(it->getTotalCapacityUsed()/it->totalCapacity)*100)/percFreecores);

            rackFitness element = {alpha, true,
                                   vector<int>(), &(*it)
            };

            this->insertRackSorted(fittingRacks, element);
        }
    }
    if(!fittingRacks.empty()) {
        rackFitness element = *fittingRacks.begin();
        return element.rack;
    } else
        return nullptr;
}

Rack* MinFragPolicy::allocateWorkloadsCoresOnly(vector<workload>& workloads, vector<int>& wloads, Layout& layout) {
    int cores = 0;
    for(auto it = wloads.begin(); it!= wloads.end(); ++it) {
        cores+=workloads[*it].cores;
    }

    bool scheduled = false;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        if(it->freeCores >= cores)
        {
            int percFreecores = (it->freeCores/it->cores)*100;
            int alpha = (percFreecores == 0) ? 0 : (((it->getTotalBandwidthUsed()/it->totalBandwidth)*100
                                                     +(it->getTotalCapacityUsed()/it->totalCapacity)*100)/percFreecores);

            rackFitness element = {alpha, true,
                                   vector<int>(), &(*it)
            };

            this->insertRackSorted(fittingRacks, element);
        }
    }
    if(!fittingRacks.empty()) {
        rackFitness element = *fittingRacks.begin();
        return element.rack;
    } else
        return nullptr;
}

bool MinFragPolicy::placeExecOnlyWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    Rack* scheduledRack = this->allocateCoresOnly(workloads, wloadIt, layout);
    if(scheduledRack != nullptr) {
        scheduledRack->freeCores -= wload->cores;
        wload->timeLeft = wload->executionTime;
        wload->allocation.coresAllocatedRack = scheduledRack;
        return true;
    } else
        return false;
}

bool MinFragPolicy::placeWorkloadInComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    vector<nvmeFitness> fittingCompositions;
    bool scheduled = false;
    for(vector<Rack>::iterator it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        int position = 0;
        for(int i = 0; i<it->compositions.size() && it->resources.begin()->getTotalCapacity()>1; ++i) {
            if(it->compositions[i].used && it->possibleToColocate(workloads, wloadIt, i, step, this->model)) {
                int wlTTL = wload->executionTime;
                int compositionTTL = it->compositionTTL(workloads, i, step);
                int compositionTotalBw = it->compositions[i].composedNvme.getTotalBandwidth();
                int compositionAvailBw = it->compositions[i].composedNvme.getAvailableBandwidth();
                int estimateTTL = wlTTL + step;

                if ((deadline==-1 || estimateTTL <= deadline) &&
                    ((wload->wlName == "smufin" && it->compositions[i].workloadsUsing<7) ||
                     (wload->wlName != "smufin" && it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
                     it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity))) {

                    int alpha = (((wload->nvmeBandwidth/compositionTotalBw)*100
                            +(wload->nvmeCapacity/it->compositions[i].composedNvme.getTotalCapacity())*100));

                    nvmeFitness element = {
                            alpha,
                            estimateTTL - compositionTTL, i, &(*it)
                    };
                    this->insertSorted(fittingCompositions, element);
                }
            }
        }
    }
    Rack* coresRack = this->allocateCoresOnly(workloads, wloadIt, layout);

    if(!fittingCompositions.empty() && coresRack != nullptr) {
        vector<nvmeFitness>::iterator it = fittingCompositions.begin();
        this->updateRackWorkloads(workloads, wloadIt, it->rack, it->rack->compositions[it->composition],
                                  it->composition);
        scheduled = true;
        coresRack->freeCores-=wload->cores;
        wload->allocation.coresAllocatedRack = coresRack;
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
        if( it->resources.begin()->getTotalCapacity()>1) {
            vector<int> selection = this->MinFragHeuristic(it->resources, it->freeResources, bandwidth, capacity);
            if (!selection.empty()) {
                int alpha = (((wload->nvmeBandwidth / it->totalBandwidth) * 100
                              + (wload->nvmeCapacity / it->totalCapacity) * 100));

                rackFitness element = {alpha, it->inUse(),
                                       selection, &(*it)
                };
                insertRackSorted(fittingRacks, element);
            }
        }
    }

    Rack* coresRack = this->allocateCoresOnly(workloads, wloadIt, layout);
    if(!fittingRacks.empty() && coresRack != nullptr) {
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
        scheduledRack->compositions[freeComposition].composedNvme = NvmeResource(composedBandwidth,composedCapacity);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(composedBandwidth-bandwidth);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(composedCapacity-capacity);
        scheduledRack->compositions[freeComposition].volumes = element.selection;
        scheduledRack->compositions[freeComposition].workloadsUsing = 1;
        scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(wloadIt);
        coresRack->freeCores -= wload->cores;
        wload->executionTime = this->model.timeDistortion(scheduledRack->compositions[freeComposition],*wload);
        wload->timeLeft = wload->executionTime;
        wload->allocation.composition = freeComposition;
        wload->allocation.allocatedRack = scheduledRack;
        wload->allocation.coresAllocatedRack = coresRack;
    }

    return scheduled;
}

bool MinFragPolicy::placeWorkloadsNewComposition(vector<workload>& workloads, vector<int>& wloads, Layout& layout, int step) {
    int capacity = 0;
    int bandwidth = 0;
    int cores = 0;
    bool smufin = false;
    int shortestDeadline = -1;
    for(auto it = wloads.begin(); it!= wloads.end(); ++it) {
        smufin = (workloads[*it].wlName == "smufin");
        capacity+=workloads[*it].nvmeCapacity;
        bandwidth+=workloads[*it].nvmeBandwidth;
        cores+=workloads[*it].cores;
        if(shortestDeadline == -1 || workloads[*it].deadline < shortestDeadline)
            shortestDeadline = workloads[*it].deadline;
    }
    assert(shortestDeadline != -1);

    if(smufin) {
        int minBw = workloads[*wloads.begin()].nvmeBandwidth;
        if ((this->model.smufinModel(minBw, wloads.size()) + step) <= shortestDeadline) {
            bandwidth = minBw;
        } else {
            //Assuming all NVMe equal
            int bwMultiple = layout.racks.end()->resources.begin()->getTotalBandwidth();
            int maxBw = bwMultiple * layout.racks.end()->resources.size();
            if((this->model.smufinModel(maxBw, wloads.size())+step) > shortestDeadline ) {
                //Will never be able to meet request!
                bandwidth = maxBw;
            } else {
                bool found = false;
                for (int i = bwMultiple; !found && i > 0; i += bwMultiple) {
                    int modelTime = this->model.smufinModel(i, wloads.size()) + step;
                    if (modelTime <= shortestDeadline) {
                        found = true;
                        bandwidth = i;
                    }
                }
            }
        }
    } else {
        int maxBw = layout.racks.end()->resources.size()*layout.racks.end()->resources.begin()->getTotalBandwidth();
        if(bandwidth>0 && bandwidth>maxBw)
            bandwidth=maxBw;
    }

    bool scheduled = false;
    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        if(it->resources.begin()->getTotalCapacity()>1) {
            vector<int> selection = this->MinFragHeuristic(it->resources, it->freeResources, bandwidth, capacity);
            if (!selection.empty()) {
                int alpha = (((bandwidth / it->totalBandwidth) * 100
                              + (capacity / it->totalCapacity) * 100));

                rackFitness element = {alpha, it->inUse(),
                                       selection, &(*it)
                };

                insertRackSorted(fittingRacks, element);
            }
        }
    }

    Rack* coresRack = this->allocateWorkloadsCoresOnly(workloads, wloads, layout);
    if(!fittingRacks.empty() && coresRack != nullptr) {
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
        coresRack->freeCores -= cores;
//        scheduledRack->freeCores -= cores;
        for(auto it = wloads.begin(); it!=wloads.end(); ++it) {
            workload* wload = &workloads[*it];
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(*it);
            wload->executionTime = this->model.timeDistortion(scheduledRack->compositions[freeComposition],*wload);
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
            wload->allocation.coresAllocatedRack = coresRack;
        }
    }

    return scheduled;
}