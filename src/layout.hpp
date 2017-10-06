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
};

#endif
