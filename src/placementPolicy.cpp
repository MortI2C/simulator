#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
#include "placementPolicy.hpp"
#include "degradationModel.hpp"
#include <cmath>
using namespace std;

PlacementPolicy::PlacementPolicy(DegradationModel degradationModel) {
    this->model = degradationModel;
}

void PlacementPolicy::freeResources(vector<workload>& workloads, int wloadIt) {
    workload* wload = &workloads[wloadIt];

    assert(wload->allocation.composition != -1 || (wload->nvmeBandwidth == 0 && wload->nvmeCapacity == 0));
    wload->allocation.coresAllocatedRack->freeCores += wload->cores;
    if(wload->allocation.composition != -1) {
        wload->allocation.allocatedRack->compositions
        [wload->allocation.composition].composedNvme
                .setAvailableBandwidth(wload->allocation.allocatedRack->compositions
                                       [wload->allocation.composition].composedNvme
                                               .getAvailableBandwidth() + wload->nvmeBandwidth);

        wload->allocation.allocatedRack->compositions
        [wload->allocation.composition].composedNvme
                .setAvailableCapacity(wload->allocation.allocatedRack->compositions
                                      [wload->allocation.composition].composedNvme
                                              .getAvailableCapacity() + wload->nvmeCapacity);

        //Remove Workload from assigned compositions Wloads
        bool found = false;
        for (auto i = wload->allocation.allocatedRack->
                compositions[wload->allocation.composition].assignedWorkloads.begin();
             !found && i != wload->allocation.allocatedRack->
                     compositions[wload->allocation.composition].assignedWorkloads.end();
             ++i) {
            workload ptWload = workloads[*i];
            if (ptWload.wlId == wload->wlId) {
                found = true;
                wload->allocation.allocatedRack->compositions[wload->allocation.composition].assignedWorkloads.erase(i);
            }
        }

        //if no more workloads in comopsition, free composition
        if ((--wload->allocation.allocatedRack->compositions[wload->allocation.composition].workloadsUsing) == 0) {
            wload->allocation.allocatedRack->freeComposition(wload->allocation.allocatedRack,
                                                             wload->allocation.composition);
        } else {
            //Update other workloads times
            this->updateRackWorkloadsTime(workloads,
                                          wload->allocation.allocatedRack->compositions[wload->allocation.composition]);
        }
    }

    if(wload->wlType == "gpuOnly") {
        //check if vgpu or phys gpu
        //free resources, if vgpu check if last using (free vgpu).
        if(wload->allocation.coresAllocatedRack->vgpus.size() > 0) {
            vGPUResource* wloadvGPU = wload->allocation.vgpu;
            bool found = false;
            for(auto it = wload->allocation.coresAllocatedRack->vgpus.begin();
                !found && it!=wload->allocation.coresAllocatedRack->vgpus.end(); ++it) {
                if(wloadvGPU == (*it)) {
                    found = true;
                    wload->allocation.coresAllocatedRack->vgpus.erase(it);
                }
            }
            GpuResource* physGpu = wloadvGPU->getPhysicalGpu();
            wloadvGPU->removeWorkload(wload);
            if(physGpu->getNumWorkloads()>0)
                wloadvGPU->setUsed(false);

            wload->allocation.vgpu = nullptr;
        } else {
            assert(wload->allocation.coresAllocatedRack->gpus.size()>0);
            bool found = false;
            for(auto it = wload->allocation.coresAllocatedRack->gpus.begin();
                !false && it!=wload->allocation.coresAllocatedRack->gpus.end(); ++it) {
                found = it->removeWorkload(wload);
            }
        }
    }
//    wload->allocation.allocatedRack->freeCores += wload->cores;
    wload->allocation = allocatedResources();
}

vector<int> PlacementPolicy::MinSetHeuristic(vector<NvmeResource>& resources, vector<int> freeResources, int bw, int cap) {
    vector<int> sortBw;
    vector<int> sortCap;
    int totalBandwidth = 0;
    int totalCapacity = 0;
    for(int i = 0; i<freeResources.size(); ++i) {
        if(freeResources[i]) {
            this->insertSortedBandwidth(resources, sortBw, i);
            this->insertSortedCapacity(resources, sortCap, i);
            totalBandwidth += resources[i].getTotalBandwidth();
            totalCapacity += resources[i].getTotalCapacity();
        }
    }

    if(totalBandwidth < bw || totalCapacity < cap) {
        return vector<int>();
    }

    int bBw = 0;
    int bCap = 0;
    vector<int> candidatesBw;
    for(auto it = sortBw.begin(); bBw < bw && it!=sortBw.end(); ++it) {
        candidatesBw.push_back(*it);
        bBw += resources[*it].getTotalBandwidth();
        bCap += resources[*it].getTotalCapacity();
    }

    if(bCap >= cap)
        return candidatesBw;

    int cBw = 0;
    int cCap = 0;
    vector<int> candidatesCap;
    for(auto it = sortCap.begin(); cCap < cap && it!=sortCap.end(); ++it) {
        candidatesCap.push_back(*it);
        cBw += resources[*it].getTotalBandwidth();
        cCap += resources[*it].getTotalCapacity();
    }

    if(cBw >= bw)
        return candidatesCap;

    if(candidatesBw.size() >= candidatesCap.size()) {
        for(auto it = sortCap.begin(); (bBw < bw || bCap < cap) && it!=sortCap.end(); ++it) {
            if(find(candidatesBw.begin(),candidatesBw.end(),*it) == candidatesBw.end()) {
                bCap += resources[*it].getTotalCapacity();
                bBw += resources[*it].getTotalBandwidth();
                candidatesBw.push_back(*it);
            }
        }

        return candidatesBw;
    } else {
        for(auto it = sortBw.begin(); (cCap < cap || cBw < bw) && it!=sortBw.end(); ++it) {
            if(find(candidatesCap.begin(),candidatesCap.end(),*it) == candidatesCap.end()) {
                cBw += resources[*it].getTotalBandwidth();
                cCap += resources[*it].getTotalCapacity();
                candidatesCap.push_back(*it);
            }
        }

        return candidatesCap;
    }
}

vector<int> PlacementPolicy::MinFragHeuristic(vector<NvmeResource>& resources, vector<int> freeResources, int bw, int cap) {
    vector<int> sortBw;
    vector<int> sortCap;
    int totalBandwidth = 0;
    int totalCapacity = 0;
    int resourceBw = -1;
    int resourceCap = -1;
    for(int i = 0; i<freeResources.size(); ++i) {
        if(freeResources[i]) {
            this->insertSortedBandwidth(resources, sortBw, i);
            this->insertSortedCapacity(resources, sortCap, i);
            totalBandwidth += resources[i].getTotalBandwidth();
            totalCapacity += resources[i].getTotalCapacity();
            if(resourceBw == -1 && resources[i].getTotalBandwidth() >= bw) {
                resourceBw = i;
            }
            if(resourceCap == -1 && resources[i].getTotalCapacity() >= cap) {
                resourceCap = i;
            }
        }
    }

    if(totalBandwidth < bw || totalCapacity < cap) {
        return vector<int>();
    }

    int bBw = 0;
    int bCap = 0;
    vector<int> candidatesBw;
    for(auto it = sortBw.begin(); bBw < bw && it!=sortBw.end(); ++it) {
        if(resourceBw == -1 || (resourceBw != -1 && resources[*it].getTotalBandwidth() >= bw)) {
            candidatesBw.push_back(*it);
            bBw += resources[*it].getTotalBandwidth();
            bCap += resources[*it].getTotalCapacity();
        }
    }

    if(bCap >= cap && bBw >= bw)
        return candidatesBw;

    int cBw = 0;
    int cCap = 0;
    vector<int> candidatesCap;
    for(auto it = sortCap.begin(); cCap < cap && it!=sortCap.end(); ++it) {
        if(resourceCap == -1 || (resourceCap != -1 && resources[*it].getTotalCapacity() >= cap)) {
            candidatesCap.push_back(*it);
            cBw += resources[*it].getTotalBandwidth();
            cCap += resources[*it].getTotalCapacity();
        }
    }

    if(cBw >= bw && cCap >= cap)
        return candidatesCap;

    if(candidatesBw.size() >= candidatesCap.size()) {
        for(auto it = sortCap.begin(); (bBw < bw || bCap < cap) && it!=sortCap.end(); ++it) {
            if(find(candidatesBw.begin(),candidatesBw.end(),*it) == candidatesBw.end()) {
                bCap += resources[*it].getTotalCapacity();
                bBw += resources[*it].getTotalBandwidth();
                candidatesBw.push_back(*it);
            }
        }

        return candidatesBw;
    } else {
        for(auto it = sortBw.begin(); (cCap < cap || cBw < bw) && it!=sortBw.end(); ++it) {
            if(find(candidatesCap.begin(),candidatesCap.end(),*it) == candidatesCap.end()) {
                cBw += resources[*it].getTotalBandwidth();
                cCap += resources[*it].getTotalCapacity();
                candidatesCap.push_back(*it);
            }
        }

        return candidatesCap;
    }
}

void PlacementPolicy::insertSortedBandwidth(vector<NvmeResource>& resources, vector<int>& vect, int element) {
    NvmeResource resource = resources[element];
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        NvmeResource current = resources[*it];
        if(resource.getTotalBandwidth() < current.getTotalBandwidth()) {
            vect.insert(it,element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

void PlacementPolicy::insertSortedCapacity(vector<NvmeResource>& resources, vector<int>& vect, int element) {
    NvmeResource resource = resources[element];
    bool inserted = false;
    for(auto it = vect.begin(); !inserted && it!=vect.end(); ++it) {
        NvmeResource current = resources[*it];
        if(resource.getTotalCapacity() < current.getTotalCapacity()) {
            vect.insert(it,element);
            inserted = true;
        }
    }
    if(!inserted)
        vect.push_back(element);
}

void PlacementPolicy::updateRackWorkloadsUnaware(vector <workload>& workloads, int wloadIt, Rack* rack, raid& composition, int compositionId) {
    workload* wload = &workloads[wloadIt];
    composition.composedNvme.setAvailableCapacity(
            (composition.composedNvme.getAvailableCapacity() - wload->nvmeCapacity));
    composition.composedNvme.setAvailableBandwidth(
            (composition.composedNvme.getAvailableBandwidth()-wload->nvmeBandwidth)
            );

    wload->allocation.composition = compositionId;
    wload->allocation.allocatedRack = rack;
    composition.workloadsUsing++;
    wload->timeLeft = wload->executionTime;

//    wload->nvmeBandwidth = (composition.composedNvme.getAvailableBandwidth() > wload->limitPeakBandwidth) ? wload->limitPeakBandwidth : composition.composedNvme.getAvailableBandwidth();
    wload->executionTime = wload->timeLeft;
//    this->updateRackWorkloadsTime(workloads, composition);
    composition.assignedWorkloads.push_back(wloadIt);
}

void PlacementPolicy::updateRackWorkloads(vector <workload>& workloads, int wloadIt, Rack* rack, raid& composition, int compositionId) {
    workload* wload = &workloads[wloadIt];
    composition.composedNvme.setAvailableCapacity(
            (composition.composedNvme.getAvailableCapacity() - wload->nvmeCapacity));
    if(wload->wlName != "smufin")
        composition.composedNvme.setAvailableBandwidth(
                (composition.composedNvme.getAvailableBandwidth()-wload->nvmeBandwidth)
        );

    wload->allocation.composition = compositionId;
    wload->allocation.allocatedRack = rack;
    composition.workloadsUsing++;
    wload->timeLeft = wload->executionTime;

    if(wload->nvmeBandwidth > 0) {
        wload->timeLeft = this->model.timeDistortion(composition,workloads[wloadIt]);
    }

//    wload->nvmeBandwidth = (composition.composedNvme.getAvailableBandwidth() > wload->limitPeakBandwidth) ? wload->limitPeakBandwidth : composition.composedNvme.getAvailableBandwidth();
    wload->executionTime = wload->timeLeft;
//    this->updateRackWorkloadsTime(workloads, composition);
    composition.assignedWorkloads.push_back(wloadIt);
}

void PlacementPolicy::updateRackWorkloadsTime(vector<workload>& workloads, raid& composition) {
    for(auto iw = composition.assignedWorkloads.begin();
        iw != composition.assignedWorkloads.end(); ++iw) {
        workload it2 = workloads[*iw];
        if(it2.nvmeBandwidth > 0) {
            int newTime = this->model.timeDistortion(composition,it2);
            it2.timeLeft = ((float) it2.timeLeft / it2.executionTime) * newTime;
            it2.executionTime = newTime;
        }
    }
}

void PlacementPolicy::updateRackGpuWorkloads(vector<workload*> workloads, int step) {
    int numConcurrent = workloads.size();

    for(auto it = workloads.begin(); it!=workloads.end(); ++it) {
        workload* wload = (*it);
        if(wload->timeLeft==wload->executionTime)
            wload->executionTime = this->model.yoloModel(numConcurrent);
        else {
            int baseTime = this->model.yoloModel(numConcurrent);
            double penalty = baseTime/wload->baseExecutionTime;
            double percentage = 1-(wload->timeLeft/wload->executionTime);
            penalty*=percentage;
            int newTime = wload->timeLeft * (1+penalty);
            if((newTime+step) <= ((wload->scheduled+baseTime)))
                wload->timeLeft *= (1+penalty);
//            cerr << wload->timeLeft << " " << penalty << " " << wload->timeLeft*penalty << endl;
        }
    }
}

bool PlacementPolicy::placeGpuOnlyWorkload(vector<workload>& workloads, int wloadIt, Layout& layout, int step, int deadline = -1) {
    workload* wload = &workloads[wloadIt];
    Rack* fittingRack = nullptr;
    //1st: look for vgpu avail on list
    if(!layout.disaggregated) {
        for(vector<Rack>::iterator it = layout.racks.begin(); fittingRack==nullptr && it!=layout.racks.end(); ++it) {
            if(!it->gpus.empty() && it->freeCores >= wload->cores) {
                vector<GpuResource>::iterator gpu = it->possiblePhysGPUAllocation(wload->gpuBandwidth,wload->gpuMemory);
                if(gpu!=it->gpus.end()) {
                    assert(gpu != it->gpus.end());
                    assert(!gpu->isUsed());
                    fittingRack = &(*it);
                    gpu->setUsed(true);
                    gpu->assignWorkload(wload);
                    assert(gpu->getNumWorkloads()==1);
//                    wload->executionTime = this->model.yoloModel(gpu->getNumWorkloads());
                }
            }
        }
    }

    //Find if any phys gpu can be added as vgpus into rack
    if(fittingRack == nullptr && layout.disaggregated) {
        //find if node is free to assign a new vgpu
        for(auto it = layout.racks.begin(); fittingRack== nullptr && it!=layout.racks.end(); ++it) {
            if(it->freeCores >= wload->cores)
                fittingRack = &(*it);
        }

        bool assigned = false;
        //find gpu with avail vgpus
        for(auto it = layout.rackPool.gpus.begin(); !assigned && fittingRack!=nullptr && it!=layout.rackPool.gpus.end(); ++it) {
            vGPUResource* vgpu = it->possibleAllocateWloadInvGPU(wload->gpuBandwidth, wload->gpuMemory);
            if(vgpu!=nullptr) {
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
                wload->timeLeft = wload->executionTime;
//                wload->executionTime = this->model.yoloModel(it->getNumWorkloads()); NO NEED, IT IS ALWAYS 1 CONC RUN HERE
                assigned = true;
            }
        }
        if(!assigned) fittingRack = nullptr;
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