#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include "Rack.hpp"
#include "layout.hpp"
//#include <jsoncpp/json/json.h>
#include "json.hpp"
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
    int totalBwRack = this->racks.begin()->resources.begin()->getTotalBandwidth()*this->racks.begin()->resources.size();
    int totalCapRack = this->racks.begin()->resources.begin()->getTotalCapacity()*this->racks.begin()->resources.size();
    int totalBwUsed = 0;
    int totalCapUsed = 0;
    int totalRacksUsed = 0;
    double f_nvme = 0;
    for(vector<Rack>::iterator it = this->racks.begin(); it!=this->racks.end(); ++it) {
        int rackBw = it->getTotalBandwidthUsed();
        int rackCap = it->getTotalCapacityUsed();
        if(rackBw > 0 || rackCap > 0)
            ++totalRacksUsed;

        totalBwUsed += rackBw;
        totalCapUsed += rackCap;

        f_nvme += it->calculateFragmentation();
    }
    f_nvme /= this->racks.size();

    int minResources = max(ceil((double)totalBwUsed/totalBwRack),ceil((double)totalCapUsed/totalCapRack));
    double f_rack = (double)(totalRacksUsed-minResources)/this->racks.size();

    if(f_rack == 0)
        return f_nvme;
    else
        return f_rack;
}

double Layout::resourcesUsed() {
    double resourcesUsed = 0;
    for(auto i = this->racks.begin(); i!=this->racks.end(); ++i) {
        resourcesUsed+=i->resourcesUsed();
    }
    return resourcesUsed/this->racks.size();
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
        cout << "Rack: " << rack++ << endl;
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
        for(auto it2 = this->racks.begin()->resources.begin();
            it2!=this->racks.begin()->resources.end(); ++it2) {
            bw+=it2->getTotalBandwidth();
        }
    }
    return bw;
}

int Layout::getTotalCapacity() {
    int cap = 0;
    for(auto it = this->racks.begin(); it!=this->racks.end(); ++it) {
        for(auto it2 = this->racks.begin()->resources.begin();
            it2!=this->racks.begin()->resources.end(); ++it2) {
            cap+=it2->getTotalCapacity();
        }
    }
    return cap;
}

double Layout::loadFactor(vector<workload>& workloads, vector<int>& queued, vector<int>& running) {
    int availBw = this->getTotalBandwidth();
    int availCap = this->getTotalCapacity();

    int bwRequested = 0;
    int capRequested = 0;
    for(auto it = queued.begin(); it!=queued.end(); ++it) {
        bwRequested += workloads[*it].nvmeBandwidth;
        capRequested += workloads[*it].nvmeCapacity;
    }

    for(auto it = running.begin(); it!=running.end(); ++it) {
        bwRequested += workloads[*it].nvmeBandwidth;
        capRequested += workloads[*it].nvmeCapacity;
    }

    return max((double)bwRequested/availBw,(double)capRequested/availCap);
}

double Layout::actualLoadFactor(vector<workload>& workloads, vector<int>& running) {
    int availBw = this->getTotalBandwidth();
    int availCap = this->getTotalCapacity();

    int bwRequested = 0;
    int capRequested = 0;
    for(auto it = running.begin(); it!=running.end(); ++it) {
        bwRequested += workloads[*it].nvmeBandwidth;
        capRequested += workloads[*it].nvmeCapacity;
    }

    return max((double)bwRequested/availBw,(double)capRequested/availCap);
}