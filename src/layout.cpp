#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include "Rack.hpp"
#include "layout.hpp"
//#include <jsoncpp/json/json.h>
#include "nlohmann/json.hpp"
#include "nvmeResource.hpp"
using namespace std;
//using namespace Json;
using json = nlohmann::json;

void Layout::generateLayout(string filePath) {
    ifstream i(filePath);
    json j;
    i >> j;
    int rackId = 0;
    this->racks = vector<Rack>(j.size());
    for(json::iterator it = j.begin(); it!=j.end(); ++it) {
        if(it.key()=="disagg") this->disaggregated = true;
        int totalBandwith = 0;
        int totalCapacity = 0;
        Rack newRack = Rack();
        json newArray = it.value();
        vector<NvmeResource> nvmes;
        for(json::iterator it2 = newArray["nvmes"].begin(); it2!=newArray["nvmes"].end(); ++it2) {
            NvmeResource newNvme(it2.value()["bandwidth"],it2.value()["capacity"]);
            nvmes.push_back(newNvme);
            totalBandwith += (int)it2.value()["bandwidth"];
            totalCapacity += (int)it2.value()["capacity"];
        }
        newRack.setTotalCores((int)newArray["cores"]);
        newRack.setFreeCores((int)newArray["cores"]);
        newRack.addNvmeResourceVector(nvmes);
        newRack.setTotalBandwidth(totalBandwith);
        newRack.setTotalCapacity(totalCapacity);
        newRack.rackId = rackId;
//      newRack.stabilizeContainers();
        this->racks[rackId++] = newRack;
    }
}

double Layout::calculateFragmentation() {
    double racksFragAccumulated = 0;
    int totalBwRack = 0;
    int totalCapRack = 0;
    int totalCoresRack =  0;
    int totalBwUsed = 0;
    int totalCapUsed = 0;
    int totalCoresUsed = 0;
    int totalRacksUsed = 0;
    double f_nvme = 0;
    for(vector<Rack>::iterator it = this->racks.begin(); it!=this->racks.end(); ++it) {
        int rackBw = it->getTotalBandwidthUsed();
        int rackCap = it->getTotalCapacityUsed();
        totalBwRack += rackBw;
        totalCapRack += rackCap;
        totalCoresRack = it->cores;

        if(rackBw > 0 || rackCap > 0 || it->freeCores < it->cores)
            ++totalRacksUsed;

        totalCoresUsed += (it->cores - it->freeCores);
        totalBwUsed += rackBw;
        totalCapUsed += rackCap;

        f_nvme += it->calculateFragmentation();
    }
    f_nvme /= this->racks.size();

    int minResources = max((double)totalCoresUsed/totalCoresRack,max(ceil((double)totalBwUsed/totalBwRack),ceil((double)totalCapUsed/totalCapRack)));
    double f_rack = (double)(totalRacksUsed-minResources)/this->racks.size();

    if(f_rack == 0 && (totalBwUsed > 0 || totalCapUsed > 0))
        return f_nvme;
    else
        return f_rack;
}

double Layout::resourcesUsed() {
    double resourcesUsed = 0;
    int totalResources = 0;
    for(auto i = this->racks.begin(); i!=this->racks.end(); ++i) {
        if(i->resources.begin()->getTotalCapacity()>1) {
            resourcesUsed += (i->resourcesUsed() * i->resources.size());
            totalResources += i->resources.size();
        }
    }
    return resourcesUsed/totalResources;
}

int Layout::raidsUsed() {
    int raidsUsed = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used) {
                raidsUsed++;
            }
        }
    }
    return raidsUsed;
}

double Layout::avgRaidSize() {
    int raidsUsed = 0;
    double avgSize = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used) {
                raidsUsed++;
                avgSize+=it->compositions[i].volumes.size();
            }
        }
    }
    return avgSize/raidsUsed;
}

double Layout::workloadsRaid() {
    int raidsUsed = 0;
    double avgSize = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used) {
                raidsUsed++;
                avgSize+=it->compositions[i].workloadsUsing;
            }
        }
    }
    return avgSize/raidsUsed;
}

void Layout::printRaidsInfo() {
    int rack = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        int usedResources = 0;
        for(int i = 0; i<it->freeResources.size(); ++i)
            if(!it->freeResources[i]) usedResources++;

        cout << "Rack: " << it->rackId << " used resources: " << usedResources << endl;
        for(int i = 0; i<it->compositions.size(); ++i) {
            if(it->compositions[i].used) {
                cout << "Total: " << it->compositions[i].composedNvme.getTotalBandwidth() << "/" <<
                     it->compositions[i].composedNvme.getTotalCapacity() << " Avail: " <<
                     it->compositions[i].composedNvme.getAvailableBandwidth() << "/" <<
                     it->compositions[i].composedNvme.getAvailableCapacity() <<
                     " num volumes: " << it->compositions[i].volumes.size() << " workloads: " <<
                     it->compositions[i].workloadsUsing << endl;
            }
        }
    }
    cout << endl;
}

int Layout::getTotalBandwidth() {
    int bw = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        for(auto it2 = it->resources.begin();
            it2!=it->resources.end(); ++it2) {
            bw = (it2->getTotalBandwidth() > 1 ) ? bw + it2->getTotalBandwidth() : bw;
        }
    }
    return bw;
}

int Layout::getTotalCapacity() {
    int cap = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        for(auto it2 = it->resources.begin();
            it2!=it->resources.end(); ++it2) {
            cap = (it2->getTotalCapacity() > 1 ) ? cap + it2->getTotalCapacity() : cap;
        }
    }
    return cap;
}

int Layout::getTotalCores() {
    int cores = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        cores+=it->getTotalCores();
    }
    return cores;
}

int Layout::getFreeCores() {
    int freeCores = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        freeCores+=it->freeCores;
    }
    return freeCores;
}

double Layout::averageCompositionSize() {
    int totalCompositions = 0;
    int compositionsSize = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        for(auto it2 = it->compositions.begin(); it2!=it->compositions.end(); ++it2) {
            if(it2->used) {
                compositionsSize += it2->volumes.size();
                totalCompositions++;
            }
        }
    }
    return (totalCompositions == 0) ? 1 : compositionsSize/totalCompositions;
}

double Layout::averageWorkloadsSharing() {
    int totalResources = 0;
    int workloadsComposition = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        for(auto it2 = it->compositions.begin(); it2!=it->compositions.end(); ++it2) {
            if(it2->used) {
                workloadsComposition += it2->workloadsUsing;
                totalResources++;
            }
        }
    }
    return (totalResources == 0)  ? 0 : workloadsComposition/totalResources;
}

double Layout::loadFactor(vector<workload>& workloads, vector<int>& queued, vector<int>& running) {
    int availBw = this->getTotalBandwidth();
    int availCap = this->getTotalCapacity();
    int availCores = this->getTotalCores();

    int bwRequested = 0;
    int capRequested = 0;
    int coresRequested = 0;
    for(auto it = queued.begin(); it!=queued.end(); ++it) {
        bwRequested += workloads[*it].nvmeBandwidth;
        capRequested += workloads[*it].nvmeCapacity;
        coresRequested += workloads[*it].cores;
    }

    for(auto it = running.begin(); it!=running.end(); ++it) {
        bwRequested += workloads[*it].nvmeBandwidth;
        capRequested += workloads[*it].nvmeCapacity;
        coresRequested += workloads[*it].cores;
    }

    return max(max((double)bwRequested/availBw,(double)capRequested/availCap),(double)coresRequested/availCores);
}

double Layout::actualLoadFactor(vector<workload>& workloads, vector<int>& running) {
    int availBw = this->getTotalBandwidth();
    int availCap = this->getTotalCapacity();
    int coresCap = this->getTotalCores();

    int bwRequested = 0;
    int capRequested = 0;
    int coresRequested = 0;
    for(auto it = running.begin(); it!=running.end(); ++it) {
        bwRequested += workloads[*it].nvmeBandwidth;
        capRequested += workloads[*it].nvmeCapacity;
        coresRequested += workloads[*it].cores;
    }
    coresCap+=coresRequested;

    return max(max((double)bwRequested/availBw,(double)capRequested/availCap),(double)coresRequested/coresCap);
}

double Layout::abstractLoadFactor(vector <workload> & workloads, vector<int> & queued){
    int availBw = this->getTotalBandwidth();
    int availCap = this->getTotalCapacity();
    int availCores = this->getTotalCores();

    int bwRequested = 0;
    int capRequested = 0;
    int coresRequested = 0;
    for(auto it = queued.begin(); it!=queued.end(); ++it) {
        bwRequested += workloads[*it].baseBandwidth;
        capRequested += workloads[*it].nvmeCapacity;
        coresRequested += workloads[*it].cores;
    }

    return max(max((double)bwRequested/availBw,(double)capRequested/availCap),(double)coresRequested/availCores);
}

double Layout::calculateLoadFactor() {
    int availBw = this->getTotalBandwidth();
    int availCap = this->getTotalCapacity();

    int bwRequested = 0;
    int capRequested = 0;
    for(auto it = this->racks.begin(); it!=racks.end(); ++it) {
        for(int i = 0; i < it->freeResources.size(); ++i) {
            if(!it->freeResources[i]) {
                bwRequested += (it->resources[i].getTotalBandwidth() -
                        it->resources[i].getAvailableBandwidth());
                capRequested += (it->resources[i].getTotalCapacity() -
                                it->resources[i].getAvailableCapacity());
            }
        }
    }

    return max((double)bwRequested/availBw,(double)capRequested/availCap);
}

int Layout::calculateMaxBandwidth() {
    int max = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        if(it->totalBandwidth > max)
            max = it->totalBandwidth;
    }

    return max;
}