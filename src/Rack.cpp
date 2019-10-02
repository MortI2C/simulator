#include <iostream>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "Rack.hpp"
#include "nvmeResource.hpp"
using namespace std;

void Rack::addNvmeResource(NvmeResource& nvme) {
   this->resources.push_back(nvme);
   this->freeResources.push_back(1);
}

void Rack::addNvmeResourceVector(vector<NvmeResource> nvmes) {
    this->resources = nvmes;
    this->freeResources = vector<int>(nvmes.size(),1);
    this->compositions = vector<raid>(nvmes.size());
    this->numFreeResources = nvmes.size();
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
}

double Rack::calculateFragmentation() {
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

    int minResources = max(ceil((double)totalBwUsed/totalBw),ceil((double)totalCapacityUsed/totalCapacity));
    return (double)(resourcesUsed-minResources)/numResources;
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

    return availBandwidth;
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