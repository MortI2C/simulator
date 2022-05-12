#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>
#include "math.h"
#include "combinedPolicy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

void CombinedPolicy::insertRackSorted(vector<rackFitness>& vect, rackFitness& element) {
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

void CombinedPolicy::insertRackSorted2(vector<rackFitness>& vect, rackFitness& element) {
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

void CombinedPolicy::insertRackSortedGpu(vector<rackFitness>& vect, rackFitness& element) {
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        if (it->fitness < element.fitness) {
            vect.insert(it, element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

void CombinedPolicy::insertSorted(vector<nvmeFitness>& vect, nvmeFitness& element) {
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

bool CombinedPolicy::placeGpuOnlyWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    Rack* fittingRack = nullptr;
    vector<rackFitness> fittingRacks;
    bool assigned = false;
    //1st: look for vgpu avail on list
    if(!layout.disaggregated) {
        for(vector<Rack>::iterator it = layout.racks.begin(); fittingRack==nullptr && it!=layout.racks.end(); ++it) {
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
                int f = remCores / layout.minCoresWl;
                if(it->inUse()) f=100-f;
                rackFitness element(f, true,
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
                wload->timeLeft = wload->executionTime;
                this->updateRackGpuWorkloads(vgpu->getPhysicalGpu()->getWorkloads(),step);
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
                if(bwDivisions > 3 || memDivisions > 3)
                    memDivisions = 3;

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
        wload->placementPolicy = "combined";

        return true;
    } else
        return false;
}

bool CombinedPolicy::placeWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
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

Rack* CombinedPolicy::allocateCoresOnly(vector<workload>& workloads, int wloadIt, Layout& layout) {
    workload* wload = &workloads[wloadIt];
    bool scheduled = false;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin(); !scheduled && it!=layout.racks.end(); ++it) {
        if(it->freeCores >= wload->cores)
        {
            int remCores = it->freeCores - wload->cores;
            rackFitness element = {100-(remCores / layout.minCoresWl), true,
                                   vector<int>(), &(*it)
            };

            this->insertRackSortedGpu(fittingRacks, element);
        }
    }
    if(!fittingRacks.empty()) {
        rackFitness element = *fittingRacks.begin();
        return element.rack;
    } else
        return nullptr;
}

Rack* CombinedPolicy::allocateWorkloadsCoresOnly(vector<workload>& workloads, vector<int>& wloads, Layout& layout) {
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
//            int percFreecores = (it->freeCores/it->cores)*100;
//            int alpha = (percFreecores == 0) ? 0 : (100-max((it->getTotalBandwidthUsed()/it->totalBandwidth)*100
//                                                     ,(it->getTotalCapacityUsed()/it->totalCapacity)*100)/percFreecores);
//
//            rackFitness element = {alpha, true,
//                                   vector<int>(), &(*it)
//            };
            int remCores = it->freeCores - cores;
            rackFitness element = {100-(remCores / layout.minCoresWl), true,
                                   vector<int>(), &(*it)
            };

            this->insertRackSortedGpu(fittingRacks, element);
        }
    }
    if(!fittingRacks.empty()) {
        rackFitness element = *fittingRacks.begin();
        return element.rack;
    } else
        return nullptr;
}

bool CombinedPolicy::placeExecOnlyWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    Rack* scheduledRack = this->allocateCoresOnly(workloads, wloadIt, layout);
    if(scheduledRack != nullptr) {
        scheduledRack->freeCores -= wload->cores;
        wload->timeLeft = wload->executionTime;
        wload->allocation.coresAllocatedRack = scheduledRack;
        wload->placementPolicy = "combined";
        return true;
    } else
        return false;
}

bool CombinedPolicy::placeWorkloadInComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
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

                        int remCores = 0;
                        if(wload->cores < it->compositions[i].coresRack->freeCores) {
                            int remCores = it->freeCores - wload->cores;

//                            alpha = (100 - 100 * ((wload->nvmeBandwidth / compositionTotalBw)
//                                                      + (wload->nvmeCapacity /
//                                                         it->compositions[i].composedNvme.getTotalCapacity()))
//                                               / (100 * wload->cores / it->compositions[i].coresRack->freeCores));
                            nvmeFitness element = {
                                    100-(remCores / layout.minCoresWl),
                                    estimateTTL - compositionTTL, i, &(*it)
                            };
                            this->insertSorted(fittingCompositions, element);
                        }

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
        wload->placementPolicy = "combined";
    }

    return scheduled;
}


bool CombinedPolicy::placeWorkloadNewComposition(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    bool scheduled = false;
    workload* wload = &workloads[wloadIt];
    int capacity = wload->nvmeCapacity;
    int bandwidth = wload->nvmeBandwidth;

    Rack* scheduledRack = nullptr;
    vector<rackFitness> fittingRacks;
    for(vector<Rack>::iterator it = layout.racks.begin();
        it!=layout.racks.end() && !scheduled;
        ++it) {
        if(it->cores>0 && it->resources.begin()->getTotalCapacity()>1 && it->freeCores < wload->cores) {
            if(it->getAvailableCapacity() >= wload->nvmeCapacity && it->getAvailableBandwidth() >= wload->nvmeBandwidth)
                wload->failToAllocateDueCores++;
        } else if(it->resources.begin()->getTotalCapacity()>1 && (it->cores==0 || it->freeCores >= wload->cores)) {
            if (wload->wlName == "smufin" || wload->wlName == "fio") {
//            if((this->model.smufinModel(bandwidth,1)+step)>deadline && deadline!=-1
//                && ((this->model.smufinModel(bandwidth,1)+step)*1.25<deadline)) {
                //Assuming all NVMe equal
                int bwMultiple = it->resources.begin()->getTotalBandwidth();
                bool found = false;
                int previousTime = -1;
                int freeResources = -1;
                for (auto it = layout.racks.begin(); it != layout.racks.end(); ++it) {
                    if (freeResources == -1 || freeResources < it->numFreeResources)
                        freeResources = it->numFreeResources;
                }
//                if(freeResources > 2) freeResources--;
                for (int i = bwMultiple; !found && i > 0 && i <= freeResources * bwMultiple &&
                                         i <= wload->limitPeakBandwidth; i += bwMultiple) {
                    int modelTime;
                    if(wload->wlName == "smufin") modelTime = this->model.smufinModel(i, 1) + step;
                    else modelTime = this->model.fioModel(i,1);

                    if (previousTime == -1 && modelTime <= deadline) {
                        bandwidth = i;
                        previousTime = modelTime;
                    } else if (previousTime > modelTime && modelTime <= deadline) {
                        bandwidth = i;
                        previousTime = modelTime;
                    } else
                        found = true;
                }
//            }
            } else  {
                float bwExtra = (deadline == -1 || bandwidth == 0) ? 0 : (
                                                                                 (log((float) (
                                                                                         (float) (deadline - step) /
                                                                                         (float) wload->executionTime)) /
                                                                                  log((float) wload->performanceMultiplier)) +
                                                                                 1
                                                                         ) * wload->baseBandwidth;
                if(bwExtra<0) bwExtra*=-1;
                if (deadline != -1 && bwExtra >= 1 && bwExtra >= wload->nvmeBandwidth) {
                    if (bwExtra > wload->limitPeakBandwidth) {
                        int newTime =
                                pow(wload->performanceMultiplier,
                                    (wload->limitPeakBandwidth / wload->baseBandwidth) - 1) *
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

            vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, bandwidth, capacity);
            if (selection.empty() && bandwidth > wload->nvmeBandwidth) {
                //try new selection
                bandwidth = wload->nvmeBandwidth;
                vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, wload->nvmeBandwidth,
                                                              capacity);
            }
            if (!selection.empty()) {
                int bwSel = 0;
                for (auto it2 = selection.begin(); it2 != selection.end(); ++it2) {
                    bwSel += it->resources[*it2].getTotalBandwidth();
                }
                raid tempComposition;
                tempComposition.composedNvme.setTotalBandwidth(bwSel);
                tempComposition.composedNvme.setAvailableBandwidth(bwSel);
                tempComposition.workloadsUsing = 1;

                int ttl = this->model.timeDistortion(tempComposition,
                                                     *wload);

                rackFitness element(ttl, it->inUse(), selection, &(*it));
                insertRackSorted(fittingRacks, element);
            }
        }
    }

    Rack* coresRack = this->allocateCoresOnly(workloads, wloadIt, layout);
    if(!fittingRacks.empty() && coresRack == nullptr) {
        wload->failToAllocateDueCores++;
    }

    if(!fittingRacks.empty() && coresRack != nullptr) {
        rackFitness element = *fittingRacks.begin();
        scheduledRack = element.rack;
        if(scheduledRack->cores>0)
            coresRack=scheduledRack;

        scheduled = true;
        int freeComposition = -1;
        for(int i = 0; freeComposition == -1 && i<scheduledRack->compositions.size(); ++i) {
            if(!scheduledRack->compositions[i].used)
                freeComposition = i;
        }

        int composedBandwidth = 0;
        int composedCapacity = 0;
        for(auto it = element.selection.begin(); it!=element.selection.end(); ++it) {
//        for(int i = 0; i<element.selection.size(); ++i) {
            scheduledRack->freeResources[*it] = 0;
            composedBandwidth += scheduledRack->resources[*it].getTotalBandwidth();
            composedCapacity += scheduledRack->resources[*it].getTotalCapacity();
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
        scheduledRack->compositions[freeComposition].coresRack = coresRack;
        coresRack->freeCores -= wload->cores;
        wload->executionTime = this->model.timeDistortion(scheduledRack->compositions[freeComposition],*wload);
        wload->timeLeft = wload->executionTime;
        wload->allocation.composition = freeComposition;
        wload->allocation.allocatedRack = scheduledRack;
        wload->allocation.coresAllocatedRack = coresRack;
        wload->placementPolicy = "combined";
    }

    return scheduled;
}

bool CombinedPolicy::placeWorkloadsNewComposition(vector<workload>& workloads, vector<int>& wloads, Layout& layout, int step) {
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
        int peakBw = workloads[*wloads.begin()].limitPeakBandwidth;
        if ((this->model.smufinModel(minBw, wloads.size()) + step) <= shortestDeadline) {
            bandwidth = minBw;
        } else {
            //Assuming all NVMe equal
            int bwMultiple = -1;
            int freeResources = -1;
            for(auto it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
                if(it->resources.begin()->getTotalBandwidth()>1 && bwMultiple == -1) {
                    bwMultiple = it->resources.begin()->getTotalBandwidth();
                    if (freeResources == -1 || freeResources < it->numFreeResources)
                        freeResources = it->numFreeResources;
                }
            }
//            if(freeResources > 2) freeResources--;

            bool found = false;
            int previousTime = -1;
            for (int i = bwMultiple; !found && i > 0 && i <= freeResources*bwMultiple && i <= peakBw; i += bwMultiple) {
                int modelTime = this->model.smufinModel(i, wloads.size()) + step;
                if (previousTime == -1 && modelTime <= deadline) {
                    bandwidth = i;
                    previousTime = modelTime;
                } else if(previousTime > modelTime && modelTime <= deadline) {
                    bandwidth = i;
                    previousTime = modelTime;
                } else
                    found = true;
            }
//            for (int i = bwMultiple; !found && i > 0; i += bwMultiple) {
//                int modelTime = this->model.smufinModel(i, wloads.size()) + step;
//                if (modelTime <= shortestDeadline) {
//                    found = true;
//                    bandwidth = i;
//                }
//            }
        }
    }

    for(auto it = layout.racks.begin(); it!=layout.racks.end(); ++it) {
        if(it->resources.begin()->getTotalCapacity() > 1 && (it->cores==0 || it->freeCores >= cores)) {
//            vector <NvmeResource> res = it->resources;
//            vector<int> sortBw;
//            for (int i = 0; i < it->freeResources.size(); ++i) {
//                if (it->freeResources[i]) {
//                    this->insertSortedBandwidth(it->resources, sortBw, i);
//                }
//            }
            vector<int> selection = this->MinSetHeuristic(it->resources, it->freeResources, bandwidth, capacity);
            if (!selection.empty()) {
                int bwSel = 0;
                for (auto it2 = selection.begin(); it2 != selection.end(); ++it2) {
                    bwSel += it->resources[*it2].getAvailableBandwidth();
                }
                int ttl = 0;
                for (auto it3 = wloads.begin(); it3 != wloads.end(); ++it3) {
                    workload *wload = &workloads[*it3];
                    if (ttl < wload->executionTime)
                        ttl = wload->executionTime;
                }

                rackFitness element(ttl, it->inUse(),selection, &(*it));
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
        for(auto it = wloads.begin(); it!=wloads.end(); ++it) {
            workload* wload = &workloads[*it];
            scheduledRack->compositions[freeComposition].assignedWorkloads.push_back(*it);
            wload->executionTime = this->model.timeDistortion(scheduledRack->compositions[freeComposition],*wload);
            wload->timeLeft = wload->executionTime;
            wload->allocation.composition = freeComposition;
            wload->allocation.allocatedRack = scheduledRack;
            wload->allocation.coresAllocatedRack = coresRack;
            wload->placementPolicy = "combined";
        }
    }

    return scheduled;
}