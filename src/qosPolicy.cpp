#include <iostream>
#include <vector>
#include <algorithm>
#include "assert.h"
#include "math.h"
#include "qosPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

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

bool QoSPolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    if(wload->nvmeBandwidth == 0 && wload->nvmeCapacity == 0) {
        return this->placeExecOnlyWorkload(workloads, wloadIt, layout, step, deadline);
    }

    if( !this->placeWorkloadInComposition(workloads,wloadIt,layout,step,deadline) ) {
        return this->placeWorkloadNewComposition(workloads, wloadIt, layout, step, deadline);
    } else
        return true;
}

bool QoSPolicy::placeExecOnlyWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    bool scheduled = false;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        if(it->freeCores >= wload->cores)
        {
            it->freeCores -= wload->cores;
            scheduled=true;
            wload->timeLeft = wload->executionTime;
            wload->allocation.allocatedRack =  &(*it);
        }
    }
    return scheduled;
}

bool QoSPolicy::placeWorkloadInComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    vector<nvmeFitness> fittingCompositions;
    bool scheduled = false;
    for(vector<Rack>::iterator it = layout.racks.begin(); it!=layout.racks.end() &&
      it->freeCores >= wload->cores; ++it) {
        int position = 0;
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used && it->possibleToColocate(workloads, wloadIt, i, step, this->model)) {
                int wlTTL = wload->executionTime + step;
                int compositionTTL = it->compositionTTL(workloads, i, step);
                int compositionTotalBw = it->compositions[i].composedNvme.getTotalBandwidth();
                int compositionAvailBw = it->compositions[i].composedNvme.getAvailableBandwidth();
                int estimateTTL = (wload->nvmeBandwidth == 0) ? wload->executionTime+step : this->model.timeDistortion(
                        it->compositions[i],*wload) + step;

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
    for(vector<Rack>::iterator it = layout.racks.begin();
        it!=layout.racks.end() && !scheduled && it->freeCores >= wload->cores;
        ++it) {

        if(wload->wlName == "smufin") {
            if((this->model.smufinModel(bandwidth,1)+step)>deadline && deadline!=-1
                && ((this->model.smufinModel(bandwidth,1)+step)*1.25<deadline)) {
                //Assuming all NVMe equal
                int bwMultiple = layout.racks.begin()->resources.begin()->getTotalBandwidth();
                bool found = false;
                int previousTime = -1;
                for (int i = bwMultiple; !found && i > 0; i += bwMultiple) {
                    int modelTime = this->model.smufinModel(i, 1) + step;
                    if (previousTime == -1 && modelTime <= deadline) {
                        bandwidth = i;
                        previousTime = modelTime;
                    } else if(previousTime > modelTime && modelTime <= deadline) {
                        bandwidth = i;
                        previousTime = modelTime;
                    } else
                        found = true;
                }
            }
        } else {
            float bwExtra = (deadline == -1 || bandwidth == 0) ? 0 : (
                    (log((float) ((float) (deadline - step) /
                    (float) wload->executionTime)) /
                    log((float) wload->performanceMultiplier)) + 1
                    ) * wload->baseBandwidth;
//        if(bwExtra>wload->baseBandwidth) cerr << bwExtra << " " << deadline << " " << step << " " << wload->executionTime << " " << wload->performanceMultiplier << " " << wload->baseBandwidth << endl;
            if (deadline != -1 && bwExtra >= 1 && bwExtra >= wload->nvmeBandwidth) {
                if (bwExtra > wload->limitPeakBandwidth) {
                    int newTime =
                            pow(wload->performanceMultiplier, (wload->limitPeakBandwidth / wload->baseBandwidth) - 1) *
                            wload->executionTime;
                    if ((step + newTime) <= deadline) {
                        bandwidth = wload->limitPeakBandwidth;
                    }
                } else {
                    int newTime = pow(wload->performanceMultiplier, bwExtra / wload->baseBandwidth - 1) *
                                  wload->executionTime;
                    if ((step + newTime) <= deadline) {
                        bandwidth = bwExtra;
                    }
                }
            }
        }

        vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, bandwidth, capacity );
        if(selection.empty() && bandwidth > wload->nvmeBandwidth) {
            //try new selection
            bandwidth = wload->nvmeBandwidth;
            vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, wload->nvmeBandwidth, capacity );
        }
        if(!selection.empty()) {
            int bwSel = 0;
            for(auto it2 = selection.begin(); it2!=selection.end(); ++it2) {
                bwSel += it->resources[*it2].getAvailableBandwidth();
            }
            raid tempComposition;
            tempComposition.composedNvme.setAvailableBandwidth(bwSel);
            tempComposition.workloadsUsing = 1;

            int ttl = this->model.timeDistortion(tempComposition,
                                             *wload);

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
        wload->executionTime = this->model.timeDistortion(scheduledRack->compositions[freeComposition],*wload);
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
            int bwMultiple = layout.racks.begin()->resources.begin()->getTotalBandwidth();
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
                if(ttl < wload->executionTime)
                    ttl = wload->executionTime;
            }

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
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(composedBandwidth-bandwidth);
        scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(composedCapacity-capacity);
        scheduledRack->compositions[freeComposition].volumes = element.selection;
        scheduledRack->compositions[freeComposition].workloadsUsing = wloads.size();
        scheduledRack->freeCores -= cores;
        for(auto it = wloads.begin(); it!=wloads.end(); ++it) {
            workload* wload = &workloads[*it];
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(*it);
            wload->executionTime = this->model.timeDistortion(scheduledRack->compositions[freeComposition],*wload);
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
        }
    }

    return scheduled;
}