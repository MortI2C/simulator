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

bool MinFragPolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step) {
    workload* wload = &workloads[wloadIt];
    vector<nvmeFitness> fittingCompositions;
    bool scheduled = false;
    for(vector<Rack>::iterator it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        int position = 0;
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used) {
                int wlTTL = wload->executionTime;
                int compositionTTL = it->compositionTTL(workloads, i, step);
                int estimateTTL = this->timeDistortion(it->compositions[i].numVolumes,
                                                       it->compositions[i].workloadsUsing + 1);
                estimateTTL+=step;

                if (it->compositions[i].workloadsUsing < maxConcurrency(it->compositions[i].numVolumes,this->loadFactor) &&
                    it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
                    it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity) {

                    nvmeFitness element = {
                            ((it->compositions[i].composedNvme.getAvailableBandwidth() - wload->nvmeBandwidth)
                             + (it->compositions[i].composedNvme.getAvailableCapacity() - wload->nvmeCapacity)),
                            estimateTTL - compositionTTL, i, &(*it)
                    };
                    if(compositionTTL <= estimateTTL)
                        this->insertSorted(fittingCompositions, element);
                }
            }
        }
    }

    if(!fittingCompositions.empty()) {
        vector<nvmeFitness>::iterator it = fittingCompositions.begin();
        it->rack->compositions[it->composition].composedNvme.setAvailableCapacity(
                (it->rack->compositions[it->composition].composedNvme.getAvailableCapacity()-wload->nvmeCapacity)
        );
        it->rack->compositions[it->composition].composedNvme.setAvailableBandwidth(
                (it->rack->compositions[it->composition].composedNvme.getAvailableBandwidth()-wload->nvmeBandwidth)
        );

        wload->allocation.composition = it->composition;
        wload->allocation.allocatedRack = it->rack;
        it->rack->compositions[it->composition].workloadsUsing++;
        wload->timeLeft = this->timeDistortion(
                it->rack->compositions[it->composition].numVolumes,
                it->rack->compositions[it->composition].workloadsUsing);
        wload->executionTime = wload->timeLeft;
        for(auto iw = it->rack->compositions[it->composition].assignedWorkloads.begin();
               iw != it->rack->compositions[it->composition].assignedWorkloads.end(); ++iw) {
            workload it2 = workloads[*iw];
            int newTime = this->timeDistortion(
                    it->rack->compositions[it->composition].numVolumes,
                    it->rack->compositions[it->composition].workloadsUsing);
//            cout <<  "before: " << it2->wlId << " " << it2->timeLeft << " ";
            it2.timeLeft = ((float)it2.timeLeft/it2.executionTime)*newTime;
            it2.executionTime = newTime;
//            cout << "after: " << it2->timeLeft << endl;
        }
        it->rack->compositions[it->composition].assignedWorkloads.push_back(wloadIt);
        scheduled = true;
    } else {
        int capacity = wload->nvmeCapacity;
        int bandwidth = wload->nvmeBandwidth;
        int nvmeBw = layout.racks.begin()->resources.begin()->getTotalBandwidth();
        int nvmeCapacity = layout.racks.begin()->resources.begin()->getTotalCapacity();
        int minResources = max(ceil((float)bandwidth/nvmeBw),ceil((float)capacity/nvmeCapacity));
//        bool extendedMode = (this->loadFactor >= 0.4);
        bool extendedMode = false;
        if(extendedMode)
            minResources++;

        Rack* scheduledRack = nullptr;
        vector<nvmeFitness> fittingRacks;
        int fitness = -1;
        for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
            if(it->inUse() && it->numFreeResources >= minResources) {
                nvmeFitness element = {it->numFreeResources - minResources,0,
                                       0, &(*it)
                };
                insertSorted(fittingCompositions, element);
                fitness = 1;
            } else if(it->numFreeResources >= minResources && scheduledRack == nullptr) {
                scheduledRack = &(*it);
                fitness = 1;
            }
        }

        if(!fittingCompositions.empty())
            scheduledRack = fittingCompositions.begin()->rack;

        if(fitness != -1) {
            scheduled = true;
            int freeComposition = -1;
            for(int i = 0; freeComposition == -1 && i<scheduledRack->compositions.size(); ++i) {
                if(!scheduledRack->compositions[i].used)
                    freeComposition = i;
            }

            int allocatedResources = 0;
            scheduledRack->numFreeResources-=minResources;
            scheduledRack->compositions[freeComposition].used = true;
//            scheduledRack->freeResources.erase(scheduledRack->freeResources.begin(),scheduledRack->freeResources.begin()+minResources-1);
            scheduledRack->compositions[freeComposition].composedNvme = NvmeResource(minResources*nvmeBw,minResources*nvmeCapacity);
            scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(minResources*nvmeBw-bandwidth);
            scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(minResources*nvmeCapacity-capacity);
            scheduledRack->compositions[freeComposition].numVolumes = minResources;
            scheduledRack->compositions[freeComposition].workloadsUsing = 1;
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(wloadIt);
            wload->executionTime = this->timeDistortion(minResources,1);
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
            int usedResources = 0;
            for(int i = 0; usedResources<minResources && i<scheduledRack->freeResources.size(); ++i) {
                if(scheduledRack->freeResources[i]) {
                    scheduledRack->freeResources[i] = 0;
                    ++usedResources;
                }
            }
        }
    }

    return scheduled;
}

void MinFragPolicy::insertSorted(vector<nvmeFitness>& vect, nvmeFitness& element) {
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