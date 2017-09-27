#include <iostream>
#include <vector>
#include <fstream>
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
      Rack newRack = Rack();
      json newArray = it.value();
      vector<NvmeResource> nvmes;
      for(json::iterator it2 = newArray["nvmes"].begin(); it2!=newArray["nvmes"].end(); ++it2) {
         NvmeResource newNvme(it2.value()["bandwidth"],it2.value()["capacity"]);
         nvmes.push_back(newNvme);
      }
      newRack.addNvmeResourceVector(nvmes);
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