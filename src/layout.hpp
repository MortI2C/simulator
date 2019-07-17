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
    vector<Rack> racks;
    Layout() {

    }

    void generateLayout(string filePath);
    double calculateFragmentation();
    double resourcesUsed();
    int raidsUsed();
    double avgRaidSize();
    double workloadsRaid();
    void printRaidsInfo();
    double averageCompositionSize();
    double loadFactor(vector<workload>&, vector<int>&, vector<int>&);
    double actualLoadFactor(vector<workload>&, vector<int>&);
    int getTotalBandwidth();
    int getTotalCapacity();
    int calculateMaxBandwidth();
};

#endif
