#include <iostream>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include "Rack.hpp"
#include "nvmeResource.hpp"
using namespace std;

void Rack::addNvmeResource(NvmeResource& nvme) {
   this->resources.push_back(nvme);
   this->freeResources.push_back(1);
}

void Rack::addNvmeResourceVector(vector<NvmeResource> nvmes) {
    this->resources = vector<NvmeResource>(nvmes.size());
    this->freeResources = vector<int>(nvmes.size());
    this->compositions = vector<raid>(nvmes.size());
    this->numFreeResources = nvmes.size();
    int i = 0;
    for(vector<NvmeResource>::iterator it = nvmes.begin(); it!=nvmes.end(); ++it, ++i) {
        this->resources[i] = *it;
        this->freeResources[i] = 1;
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
       int nvmeBw = rack->compositions[composition].composedNvme.getTotalBandwidth() /
                    rack->compositions[composition].numVolumes;
       int nvmeCapacity = rack->compositions[composition].composedNvme.getTotalCapacity() /
                          rack->compositions[composition].numVolumes;
       rack->compositions[composition].used = false;
       int freedResources = 0;
       for (int i = 0; freedResources < rack->compositions[composition].numVolumes
                       && i < rack->compositions[composition].numVolumes; ++i) {
           if (!rack->freeResources[i]) {
               rack->freeResources[i] = 1;
               ++freedResources;
           }
       }
       rack->numFreeResources+=freedResources;
}

void Rack::stabilizeContainers() {
   this->resources.resize(this->resources.size());
   this->freeResources.resize(this->freeResources.size());
   this->compositions.resize(this->freeResources.size());
}