#include <iostream>
#include <vector>
#include <algorithm>
#include "math.h"
#include "bestFitPolicy.hpp"
#include "../src/layout.hpp"
#include "../src/resources_structures.hpp"
#include "../src/nvmeResource.hpp"
using namespace std;

bool BestFitPolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step){
    workload* wload = &workloads[wloadIt];
    vector<nvmeFitness> fittingCompositions;
    bool scheduled = false;
    for(vector<Rack>::iterator it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        int position = 0;
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used && it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
                    it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity) {
                nvmeFitness element = {((it->compositions[i].composedNvme.getAvailableBandwidth()-wload->nvmeBandwidth)
                        +(it->compositions[i].composedNvme.getAvailableCapacity()-wload->nvmeCapacity)),0,
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
        it->rack->compositions[it->composition].workloadsUsing++;
        wload->timeLeft = this->model.timeDistortion(
                it->rack->compositions[it->composition].numVolumes,
                it->rack->compositions[it->composition].workloadsUsing);
        wload->executionTime = wload->timeLeft;
        for(auto iw = it->rack->compositions[it->composition].assignedWorkloads.begin();
            iw != it->rack->compositions[it->composition].assignedWorkloads.end(); ++iw) {
            workload it2 = workloads[*iw];
            int newTime = this->model.timeDistortion(
                    it->rack->compositions[it->composition].numVolumes,
                    it->rack->compositions[it->composition].workloadsUsing);
//            cout <<  "before: " << it2->wlId << " " << it2->timeLeft << " ";
            it2.timeLeft = ((float)it2.timeLeft/it2.executionTime)*newTime;
            it2.executionTime = newTime;
        }
        it->rack->compositions[it->composition].assignedWorkloads.push_back(wloadIt);

        scheduled = true;
    } else {
        int capacity = wload->nvmeCapacity;
        int bandwidth = wload->nvmeBandwidth;
        int nvmeBw = layout.racks.begin()->resources.begin()->getTotalBandwidth();
        int nvmeCapacity = layout.racks.begin()->resources.begin()->getTotalCapacity();
        int minResources = max(ceil((float)bandwidth/nvmeBw),ceil((float)capacity/nvmeCapacity));

        vector<Rack>::iterator scheduledRack;
        int fitness = -1;
        for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
            if(it->numFreeResources>=minResources) {
                if (fitness == -1 || fitness > (it->numFreeResources - minResources)) {
                    scheduledRack = it;
                    fitness = it->numFreeResources - minResources;
                }
            }
        }

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
            scheduledRack->compositions[freeComposition].composedNvme = NvmeResource(minResources*nvmeBw,minResources*nvmeCapacity);
            scheduledRack->compositions[freeComposition].composedNvme.setAvailableBandwidth(minResources*nvmeBw-bandwidth);
            scheduledRack->compositions[freeComposition].composedNvme.setAvailableCapacity(minResources*nvmeCapacity-capacity);
            scheduledRack->compositions[freeComposition].numVolumes = minResources;
            scheduledRack->compositions[freeComposition].workloadsUsing++;
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(wloadIt);
            wload->timeLeft = this->model.timeDistortion(minResources,1);
            wload->executionTime = wload->timeLeft;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = &(*scheduledRack);
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


void BestFitPolicy::insertSorted(vector<nvmeFitness>& vector, nvmeFitness element) {
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