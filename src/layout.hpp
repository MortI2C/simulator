#ifndef LAYOUT_H
#define LAYOUT_H
#include <iostream>
#include <vector>
#include <string>
#include "Rack.hpp"
#include "nvmeResource.hpp"
using namespace std;

class Layout {
   public:
    Rack rackPool;
    vector<Rack> racks;
    vector<GpuResource> gpus;
    bool disaggregated;
    int minCoresWl;
    Layout() {
        this->disaggregated = false;
    }

    void generateLayout(string filePath);
    double calculateFragmentation();
    double resourcesUsed();
    int raidsUsed();
    double avgRaidSize();
    double workloadsRaid();
    void printRaidsInfo();
    double averageCompositionSize();
    double averageWorkloadsSharing();
    double loadFactor(vector<workload>&, vector<int>&, vector<int>&);
    double actualLoadFactor(vector<workload>&, vector<int>&);
    double abstractLoadFactor(vector<workload>&, vector<int>&);
    int getTotalBandwidth();
    int getTotalCapacity();
    int getTotalCores();
    int getFreeCores();
    int getTotalGpuMemory();
    int calculateMaxBandwidth();
    double calculateLoadFactor();
    loadFactors calculateAbstractLoadFactors(vector<workload>&, vector<int>&);
    loadFactors calculateLoadFactors(vector<workload>&, vector<int>&, vector<int>&);
};

#endif
