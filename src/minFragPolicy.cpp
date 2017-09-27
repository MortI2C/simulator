#include <iostream>
#include <vector>
#include <algorithm>
#include "math.h"
#include "minFragPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

bool MinFragPolicy::scheduleWorkload(vector<workload>::iterator wload, int step, Layout& layout) {
    vector<nvmeFitness> fittingCompositions;
    bool scheduled = false;
    for(vector<Rack>::iterator it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        int position = 0;
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used && it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
                    it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity) {
                nvmeFitness element = {((it->compositions[i].composedNvme.getAvailableBandwidth()-wload->nvmeBandwidth)
                        +(it->compositions[i].composedNvme.getAvailableCapacity()-wload->nvmeCapacity)),
                        i,&(*it)
                };
                insertSorted(fittingCompositions, element);
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
        wload->allocation.workloadsUsing++;
        scheduled = true;
    } else {
        int capacity = wload->nvmeCapacity;
        int bandwidth = wload->nvmeBandwidth;
        int nvmeBw = layout.racks.begin()->resources.begin()->getTotalBandwidth();
        int nvmeCapacity = layout.racks.begin()->resources.begin()->getTotalCapacity();
        int minResources = max(ceil((float)bandwidth/nvmeBw),ceil((float)capacity/nvmeCapacity));

        Rack* scheduledRack = nullptr;
        vector<nvmeFitness> fittingRacks;
        int fitness = -1;
        for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
            if(it->inUse() && it->numFreeResources >= minResources) {
                nvmeFitness element = {it->numFreeResources - minResources,
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
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = &(*scheduledRack);
            wload->allocation.workloadsUsing = 1;
            int usedResources = 0;
            for(int i = 0; usedResources<minResources && i<scheduledRack->freeResources.size(); ++i) {
                if(scheduledRack->freeResources[i]) {
                    scheduledRack->freeResources[i] = 0;
                    ++usedResources;
                }
            }
        }
    }

    if(scheduled)
        wload->scheduled = step;

    return scheduled;
}

void MinFragPolicy::insertSorted(vector<nvmeFitness>& vector, nvmeFitness element) {
    bool inserted = false;
    for(std::vector<nvmeFitness>::iterator it = vector.begin(); !inserted && it!=vector.end(); ++it) {
        if(it->fitness >= element.fitness) {
            vector.insert(it,element);
            inserted = true;
        }
    }
    if(!inserted)
        vector.push_back(element);
}