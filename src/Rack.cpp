#include <iostream>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "assert.h"
#include "Rack.hpp"
#include "nvmeResource.hpp"
using namespace std;

void Rack::addNvmeResource(NvmeResource& nvme) {
   this->resources.push_back(nvme);
   this->freeResources.push_back(1);
}

void Rack::addNvmeResourceVector(vector<NvmeResource> nvmes) {
    this->resources = nvmes;
    if(nvmes.begin()->getTotalCapacity()>1) {
        this->freeResources = vector<int>(nvmes.size(), 1);
        this->compositions = vector<raid>(nvmes.size());
        this->numFreeResources = nvmes.size();
    } else {
        this->freeResources = vector<int>(0,0);
        this->compositions = vector<raid>(0);
        this->numFreeResources = 0;
    }
}

void Rack::deleteNvmeResource (NvmeResource* nvme) {
//   vector<NvmeResource>::iterator foundElement = this->resources.end();
//   for(vector<NvmeResource>::iterator it = this->resources.begin(); foundElement != this->resources.end() && it!=this->resources.end(); ++it) {
//      if(&(*it) == nvme) {
//         foundElement = it;
//         for(vector<raid>::iterator it2 = this->compositions.begin(); it2!=this->compositions.end(); ++it2) {
//            vector<vector<NvmeResource>::iterator>::iterator it3;
//            it3 = find(it2->volumes.begin(), it2->volumes.end(), it);
//            if(it3 != it2->volumes.end())
//               it2->volumes.erase(it3);
//         }
//      }
//   }
}

void Rack::dumpRack() {
    for(vector<NvmeResource>::iterator it = this->resources.begin(); it!=this->resources.end(); ++it) {
        cout << it->getTotalBandwidth() << " " << it->getTotalCapacity() << endl;
    }
}

void Rack::freeComposition(Rack* rack, int composition) {
       for(auto it = rack->compositions[composition].volumes.begin();
               it!=rack->compositions[composition].volumes.end(); ++it) {
           rack->freeResources[*it] = 1;
       }

       rack->compositions[composition].used = false;
       rack->numFreeResources+=rack->compositions[composition].volumes.size();
       for(auto it = rack->compositions[composition].volumes.begin();
        it!=rack->compositions[composition].volumes.end(); ++it) {
           rack->freeResources[*it] = 1;
       }

       rack->compositions[composition].volumes.erase(
               rack->compositions[composition].volumes.begin(),
               rack->compositions[composition].volumes.end()
       );
       rack->compositions[composition].assignedWorkloads.erase(
               rack->compositions[composition].assignedWorkloads.begin(),
               rack->compositions[composition].assignedWorkloads.end()
       );
       rack->compositions[composition].volumes = vector<int>(0);
       rack->compositions[composition].composedNvme = NvmeResource(0,0);
       rack->compositions[composition].coresRack = nullptr;
}

double Rack::calculateFragmentation() {
    int numResources = this->resources.size();
    int totalBwUsed = 0;
    int totalCapacityUsed = 0;
    int totalBw = (this->resources.begin()->getTotalBandwidth()*this->resources.size());
    int totalCapacity = (this->resources.begin()->getTotalCapacity()*this->resources.size());
    int totalCores = 0;
    int coresUsed = 0;
    int resourcesUsed = 0;
    vector<Rack*> racks;
    for(int i = 0; i<this->compositions.size(); ++i) {
        if(this->compositions[i].used) {
            totalCapacityUsed+=this->compositions[i].composedNvme.getTotalCapacity()-this->compositions[i].composedNvme.getAvailableCapacity();
            totalBwUsed+=this->compositions[i].composedNvme.getTotalBandwidth()-this->compositions[i].composedNvme.getAvailableBandwidth();
            resourcesUsed+=this->compositions[i].volumes.size();
            Rack* compRack = this->compositions[i].coresRack;
            assert(compRack != nullptr);
            if(std::find(racks.begin(),racks.end(),compRack)==racks.end())
                racks.push_back(compRack);
        }
    }
    for(auto it = racks.begin(); it!=racks.end(); ++it) {
        Rack* rel = *it;
        totalCores+=rel->cores;
        coresUsed+=(rel->cores-rel->freeCores);
    }

    double minResources = max(ceil((double)totalBwUsed/totalBw),ceil((double)totalCapacityUsed/totalCapacity));
    double minCores = (racks.size()>0) ? (double)coresUsed/totalCores : 0;
    if(minCores==0)
        return 0;
    else {
        double frag = (minResources<minCores) ? minResources / minCores : minCores  / minResources;
        return frag;
    }
}

double Rack::estimateFragmentation(int sumResources, int sumBw, int sumCap) {
    int numResources = this->resources.size();
    int totalBwUsed = 0;
    int totalCapacityUsed = 0;
    int totalBw = (this->resources.begin()->getTotalBandwidth());
    int totalCapacity = (this->resources.begin()->getTotalCapacity());
    int resourcesUsed = 0;
    for(int i = 0; i<this->compositions.size(); ++i) {
        if(this->compositions[i].used) {
            totalCapacityUsed+=this->compositions[i].composedNvme.getTotalCapacity()-this->compositions[i].composedNvme.getAvailableCapacity();
            totalBwUsed+=this->compositions[i].composedNvme.getTotalBandwidth()-this->compositions[i].composedNvme.getAvailableBandwidth();
            resourcesUsed+=this->compositions[i].volumes.size();
        }
    }
    resourcesUsed+=sumResources;
    totalBwUsed+=sumBw;
    totalCapacityUsed+=sumCap;

    int minResources = max(ceil(totalBwUsed/totalBw),ceil(totalCapacityUsed/totalCapacity));
    return (double)(resourcesUsed-minResources)/numResources;
}

int Rack::getTotalBandwidthUsed() {
    int numResources = this->resources.size();
    int totalBwUsed = 0;
    for(int i = 0; i<this->compositions.size(); ++i) {
        if(this->compositions[i].used)
            totalBwUsed+=this->compositions[i].composedNvme.getTotalBandwidth()-this->compositions[i].composedNvme.getAvailableBandwidth();
    }

    return totalBwUsed;
}

int Rack::getTotalCapacityUsed(){
    int totalCapacityUsed = 0;
    for(int i = 0; i<this->compositions.size(); ++i) {
        if(this->compositions[i].used)
            totalCapacityUsed+=this->compositions[i].composedNvme.getTotalCapacity()-this->compositions[i].composedNvme.getAvailableCapacity();
    }

    return totalCapacityUsed;
}

bool Rack::inUse() {
    return this->numFreeResources != this->resources.size();
}

double Rack::resourcesUsed() {
    double resourcesUsed = 0;
    for(auto i = this->freeResources.begin(); i!=this->freeResources.end(); ++i) {
        if(!*i)
            resourcesUsed++;
    }

    return resourcesUsed/this->resources.size();
}

double Rack::getAvailableBandwidth() {
    double availBandwidth = 0;
    for(int i = 0; i<this->freeResources.size(); ++i) {
        if(this->freeResources[i])
           availBandwidth += resources[i].getAvailableBandwidth();
    }
    for(auto it = this->compositions.begin(); it!= this->compositions.end(); ++it)
    {
        if(it->used)
            availBandwidth += it->composedNvme.getAvailableBandwidth();
    }

    return availBandwidth;
}

double Rack::getAvailableCapacity() {
    double availCapacity = 0;
    for(int i = 0; i<this->freeResources.size(); ++i) {
        if(this->freeResources[i])
            availCapacity += resources[i].getAvailableCapacity();
    }
    for(auto it = this->compositions.begin(); it!= this->compositions.end(); ++it)
    {
        if(it->used)
            availCapacity += it->composedNvme.getAvailableCapacity();
    }

    return availCapacity;
}

int Rack::compositionTTL(vector<workload>& workloads, int composition, int step) {
    int ttl = -1;
    for(auto it = this->compositions[composition].assignedWorkloads.begin();
            it != this->compositions[composition].assignedWorkloads.end();
            ++it) {
        workload wload = workloads[*it];
        if(wload.scheduled >= 0) {
            int wlTTL = wload.scheduled+wload.executionTime;
            if(ttl == -1 || wlTTL >= ttl)
                ttl = wlTTL;
        }
    }

    return ttl;
}

void Rack::setTotalBandwidth(int bandwidth) {
    this->totalBandwidth = bandwidth;
}

void Rack::setTotalCapacity(int capacity) {
    this->totalCapacity = capacity;
}

void Rack::setFreeCores(int freeCores) {
    this->freeCores = freeCores;
}

void Rack::setTotalCores(int cores) {
    this->cores = cores;
}

int Rack::getTotalCores() {
    return this->cores;
}

void Rack::setTotalGpuBandwidth(int bandwidth) {
    this->totalGpuBandwidth = bandwidth;
}

void Rack::setTotalGpuMemory(int memory) {
    this->totalGpuMemory = memory;
}


bool Rack::possibleToColocate(vector<workload>& workloads, int wloadId, int composition, int step, DegradationModel& model) {
    workload* wload = &workloads[wloadId];

    bool smufin = false;
   for(auto it2 = this->compositions[composition].assignedWorkloads.begin(); it2!=this->compositions[composition].assignedWorkloads.end(); ++it2) {
        if(workloads[*it2].wlName == "smufin")
            smufin = true;
    }
    if(smufin && wload->wlName != "smufin")
        return false;
    else if(!smufin && wload->wlName != "smufin")
        return true;
    else if(smufin && wload->wlName == "smufin") {
        //ALL of them are smufin
        for (auto it2 = this->compositions[composition].assignedWorkloads.begin();
             it2 != this->compositions[composition].assignedWorkloads.end(); ++it2) {
            int newTime = model.smufinModel(this->compositions[composition].composedNvme.getTotalBandwidth(),
                                            this->compositions[composition].workloadsUsing + 1);
            workload *assigned = &workloads[*it2];
            int timeLeft = ((float) assigned->timeLeft / assigned->executionTime) * newTime;
            if ((step + timeLeft) > assigned->deadline) {
                return false;
            }
        }
        return true;
    } else
        return false;
}