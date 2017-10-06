#ifndef PLACEMENT_POLICY_H
#define PLACEMENT_POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

class PlacementPolicy {
   public:
    virtual bool placeWorkload(vector<workload>::iterator, Layout&) =0;
    void freeResources(workload& wload) {
        wload.allocation.allocatedRack->compositions
        [wload.allocation.composition].composedNvme
                .setAvailableBandwidth(wload.allocation.allocatedRack->compositions
                                       [wload.allocation.composition].composedNvme
                                               .getAvailableBandwidth()+wload.nvmeBandwidth);

        wload.allocation.allocatedRack->compositions
        [wload.allocation.composition].composedNvme
                .setAvailableCapacity(wload.allocation.allocatedRack->compositions
                                      [wload.allocation.composition].composedNvme
                                              .getAvailableCapacity()+wload.nvmeCapacity);

//        //Remove Workload from assigned compositions Wloads
//        bool found = false;
//        for(auto i = wload.allocation.allocatedRack->
//                compositions[wload.allocation.composition].assignedWorkloads.begin();
//                !found && i!=wload.allocation.allocatedRack->
//                        compositions[wload.allocation.composition].assignedWorkloads.end();
//            ++i) {
//            if(*i == wloadIt) {
//                found = true;
//                wload.allocation.allocatedRack->compositions[wload.allocation.composition].assignedWorkloads.erase(i);
//            }
//        }

        //if no more workloads in comopsition, free composition
        if((--wload.allocation.allocatedRack->compositions[wload.allocation.composition].workloadsUsing)==0) {
            wload.allocation.allocatedRack->freeComposition(wload.allocation.allocatedRack, wload.allocation.composition);
            wload.allocation = {};
        }
    }
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
