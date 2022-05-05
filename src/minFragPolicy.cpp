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
        } else if (it->fitness < element.fitness) {
            vect.insert(it, element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

void MinFragPolicy::insertRackSorted2(vector<rackFitness>& vect, rackFitness& element) {
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if (it->inUse && !element.inUse) {
            vect.insert(it, element);
            inserted = true;
        }
        else if (it->fitness < element.fitness) {
            vect.insert(it, element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

void MinFragPolicy::insertRackSortedGpu(vector<rackFitness>& vect, rackFitness& element) {
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if (it->fitness > element.fitness) {
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
        if(it->fitness > element.fitness) {
            vect.insert(it,element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

bool MinFragPolicy::placeGpuOnlyWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    Rack* fittingRack = nullptr;
    vector<rackFitness> fittingRacks;
    bool assigned = false;
    //1st: look for vgpu avail on list
    for(vector<Rack>::iterator it = layout.racks.begin(); fittingRack==nullptr && it!=layout.racks.end(); ++it) {
        if(layout.disaggregated) {
            if(it->freeCores >= wload->cores && it->vgpus.size()>0) {
                for(auto it2 = it->vgpus.begin(); fittingRack==nullptr && it2!=it->vgpus.end(); ++it2) {
                    if(!(*it2)->isUsed()) {
                        int remCores = it->freeCores - wload->cores;
                        rackFitness element(remCores / layout.minCoresWl, true,
                                            vector<int>(), &(*it));
                        element.vgpu = *it2;
                        this->insertRackSortedGpu(fittingRacks, element);
                    }
                }
            }
        } else {
            if(!it->gpus.empty() && it->freeCores >= wload->cores) {
                vector<GpuResource>::iterator gpu = it->possiblePhysGPUAllocation(wload->gpuBandwidth,wload->gpuMemory);
                if(gpu!=it->gpus.end()) {
                    assert(gpu != it->gpus.end());
                    int remCores = it->freeCores - wload->cores;
                    rackFitness element = {remCores / layout.minCoresWl, true,
                                           vector<int>(), &(*it)
                    };
                    element.gpu = &(*gpu);
                    this->insertRackSortedGpu(fittingRacks, element);
                }
            }
        }
    }

    if(fittingRacks.size() == 0 && layout.disaggregated) {
        //find if node is free to assign a new vgpu
        for(auto it = layout.racks.begin(); fittingRack== nullptr && it!=layout.racks.end(); ++it) {
            if(it->freeCores >= wload->cores) {
                int remCores = it->freeCores - wload->cores;
                rackFitness element(remCores / layout.minCoresWl, true,
                                       vector<int>(), &(*it));
                this->insertRackSortedGpu(fittingRacks, element);
//                fittingRack = &(*it);
            }
        }
        if(fittingRacks.size()>0)
            fittingRack = fittingRacks.begin()->rack;

        //find gpu with avail vgpus
        for(auto it = layout.rackPool.gpus.begin(); !assigned && fittingRack!=nullptr && it!=layout.rackPool.gpus.end(); ++it) {
            vGPUResource* vgpu = it->possibleAllocateWloadInvGPU(wload->gpuBandwidth, wload->gpuMemory);
            if(vgpu!= nullptr) {
                fittingRack->addvGPU(vgpu);
                vgpu->assignWorkload(wload);
                this->updateRackGpuWorkloads(vgpu->getPhysicalGpu()->getWorkloads());
                assigned = true;
            }
        }

        //Find if phys gpu free + partition into vgpus
        for(auto it = layout.rackPool.gpus.begin(); !assigned && fittingRack!=nullptr && it!=layout.rackPool.gpus.end(); ++it) {
            if(!it->isUsed()) {
                assert(it->getTotalBandwidth()>=wload->gpuBandwidth &&
                       it->getTotalMemory()>=wload->gpuMemory);

                int bwDivisions = floor(it->getTotalBandwidth()/wload->gpuBandwidth);
                int memDivisions = floor(it->getTotalMemory()/wload->gpuMemory);
                assert(memDivisions >= 1 && bwDivisions >= 1);

                bwDivisions = it->getTotalBandwidth()/bwDivisions;
                memDivisions = it->getTotalMemory()/memDivisions;

                int totalBw = it->getTotalBandwidth();
                int totalMem = it->getTotalMemory();
                vector<vGPUResource*> vgpus;
                for(int j = 0; totalBw > 0 && totalMem > 0; ++j) {
                    int bwvGPU = (totalBw >= bwDivisions) ? bwDivisions  : totalBw;
                    int memvGPU = (totalMem >= memDivisions) ? memDivisions : totalMem;
                    assert(bwvGPU > 0 && memvGPU > 0);

                    totalBw -= bwDivisions;
                    totalMem -= memDivisions;
                    vGPUResource* vGPU = new vGPUResource(bwvGPU,memvGPU, &(*it));
                    vgpus.push_back(vGPU);
                }
                it->addVgpusVector(vgpus);
                it->setUsed(true);

                fittingRack->addvGPU(*vgpus.begin());
                (*vgpus.begin())->assignWorkload(wload);
//                wload->executionTime = this->model.yoloModel(it->getNumWorkloads());
                assigned = true;
            }
        }
        if(!assigned) {
            fittingRacks.clear();
            fittingRack = nullptr;
        }
    }

    if(fittingRacks.size()>0 && !assigned) {
        fittingRack = fittingRacks.begin()->rack;
        if(layout.disaggregated) {
            assert(fittingRacks.begin()->vgpu!= nullptr);
            fittingRacks.begin()->vgpu->assignWorkload(wload);
            wload->executionTime = this->model.yoloModel(fittingRacks.begin()->vgpu->getPhysicalGpu()->getNumWorkloads());
        } else {
            GpuResource* gpu = fittingRacks.begin()->gpu;
            assert(gpu!=nullptr);
            gpu->setUsed(true);
            gpu->assignWorkload(wload);
            wload->executionTime = this->model.yoloModel(gpu->getNumWorkloads());
        }
    }

    if(fittingRack != nullptr) {
        fittingRack->freeCores -= wload->cores;
        wload->timeLeft = wload->executionTime;
        wload->allocation.coresAllocatedRack = fittingRack;
        wload->placementPolicy = "gpu";

        return true;
    } else
        return false;
}

bool MinFragPolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    if(wload->wlType == "gpuOnly") {
        return this->placeGpuOnlyWorkload(workloads, wloadIt, layout, step, deadline);
    } else if(wload->nvmeBandwidth == 0 && wload->nvmeCapacity == 0) {
        return this->placeExecOnlyWorkload(workloads, wloadIt, layout, step, deadline);
    } else {

        workloads[wloadIt].allocationAttempts++;
        bool scheduled = this->placeWorkloadInComposition(workloads, wloadIt, layout, step, deadline);
        if (!scheduled) {
            workloads[wloadIt].allocationAttempts++;
            return this->placeWorkloadNewComposition(workloads, wloadIt, layout, step, deadline);
        }

        return scheduled;
    }
}

Rack* MinFragPolicy::allocateCoresOnly(vector<workload>& workloads, int wloadIt, Layout& layout) {
    workload* wload = &workloads[wloadIt];
    bool scheduled = false;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        if(it->freeCores >= wload->cores)
        {
//            int alpha = 100 - (it->freeCores/it->cores)*100;
            int percFreecores = (it->freeCores/it->cores)*100;
            int alpha = (percFreecores == 0) ? 0 : (100-max((it->getTotalBandwidthUsed()/it->totalBandwidth)*100
                                                     ,(it->getTotalCapacityUsed()/it->totalCapacity)*100)/percFreecores);

            rackFitness element = {alpha, true,
                                   vector<int>(), &(*it)
            };

            this->insertRackSorted2(fittingRacks, element);
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
//            int alpha = 100 - (it->freeCores/it->cores)*100;
            int percFreecores = (it->freeCores/it->cores)*100;
            int alpha = (percFreecores == 0) ? 0 : (100-max((it->getTotalBandwidthUsed()/it->totalBandwidth)*100
                                                     ,(it->getTotalCapacityUsed()/it->totalCapacity)*100)/percFreecores);

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
        wload->placementPolicy = "minfrag";
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
        if(it->resources.begin()->getTotalCapacity()>1 && (it->cores==0 || it->freeCores >= wload->cores)) {
            for (int i = 0; i < it->compositions.size(); ++i) {
                if (it->compositions[i].used && it->compositions[i].coresRack->freeCores>=wload->cores &&
                    it->possibleToColocate(workloads, wloadIt, i, step, this->model)) {
                    int wlTTL = wload->executionTime;
                    int compositionTTL = it->compositionTTL(workloads, i, step);
                    int compositionTotalBw = it->compositions[i].composedNvme.getTotalBandwidth();
                    int compositionAvailBw = it->compositions[i].composedNvme.getAvailableBandwidth();
                    int estimateTTL = wlTTL + step;

                    if ((deadline == -1 || estimateTTL <= deadline) &&
                        ((wload->wlName == "smufin" && it->compositions[i].workloadsUsing < 7) ||
                         (wload->wlName != "smufin" &&
                          it->compositions[i].composedNvme.getAvailableBandwidth() >= wload->nvmeBandwidth &&
                          it->compositions[i].composedNvme.getAvailableCapacity() >= wload->nvmeCapacity))) {

                        int alpha = 0;
                        if(wload->cores < it->compositions[i].coresRack->freeCores) {
                            alpha = (100 - 100 * ((wload->nvmeBandwidth / compositionTotalBw)
                                                      + (wload->nvmeCapacity /
                                                         it->compositions[i].composedNvme.getTotalCapacity()))
                                               / (100 * wload->cores / it->compositions[i].coresRack->freeCores));

                        }
                        nvmeFitness element = {
                                alpha,
                                estimateTTL - compositionTTL, i, &(*it)
                        };
                        this->insertSorted(fittingCompositions, element);
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

    Rack* coresRack = this->allocateCoresOnly(workloads, wloadIt, layout);
//    if(!fittingCompositions.empty() && coresRack == nullptr)
//        wload->failToAllocateDueCores++;

    if(!fittingCompositions.empty()) {
        vector<nvmeFitness>::iterator it = fittingCompositions.begin();
        coresRack = it->rack->compositions[it->composition].coresRack;

        this->updateRackWorkloads(workloads, wloadIt, it->rack, it->rack->compositions[it->composition],
                                  it->composition);
        scheduled = true;
        coresRack->freeCores-=wload->cores;
        wload->allocation.coresAllocatedRack = coresRack;
        wload->placementPolicy = "minfrag";
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
        if(it->cores>0 && it->resources.begin()->getTotalCapacity()>1 && it->freeCores < wload->cores) {
            if(it->getAvailableCapacity() >= wload->nvmeCapacity && it->getAvailableBandwidth() >= wload->nvmeBandwidth)
                wload->failToAllocateDueCores++;
        }
        else if(it->resources.begin()->getTotalCapacity()>1 && (it->cores==0 || it->freeCores >= wload->cores)) {
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
    if(!fittingRacks.empty() && coresRack == nullptr)
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
        scheduledRack->compositions[freeComposition].coresRack = coresRack;
        coresRack->freeCores -= wload->cores;
        wload->executionTime = this->model.timeDistortion(scheduledRack->compositions[freeComposition],*wload);
        wload->timeLeft = wload->executionTime;
        wload->allocation.composition = freeComposition;
        wload->allocation.allocatedRack = scheduledRack;
        wload->allocation.coresAllocatedRack = coresRack;
        wload->placementPolicy = "minfrag";
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
            int bwMultiple = -1;
            for(auto it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
                if(it->resources.begin()->getTotalBandwidth()>1 && bwMultiple == -1)
                    bwMultiple = it->resources.begin()->getTotalBandwidth();
            }

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
        if(it->resources.begin()->getTotalCapacity()>1 && (it->cores==0 || it->freeCores >= cores)) {
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
            wload->placementPolicy = "minfrag";
        }
    }

    return scheduled;
}