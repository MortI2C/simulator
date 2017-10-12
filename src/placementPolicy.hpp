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
    struct distortionValues {
        float a;
        float b;
    };
    vector<distortionValues> distortion;
    PlacementPolicy() {
        this->distortion = vector<distortionValues>(2);
        this->distortion[0].a = 1358.83;
        this->distortion[0].b = 0.14008;
        this->distortion[1].a = 1389.43;
        this->distortion[1].b = 0.490824;
    }
    virtual bool placeWorkload(vector<workload>::iterator, Layout&, int) =0;
    void freeResources(vector<workload>::iterator wloadIt) {
        workload wload = *wloadIt;
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
        bool found = false;
        for(auto i = wload.allocation.allocatedRack->
                compositions[wload.allocation.composition].assignedWorkloads.begin();
                !found && i!=wload.allocation.allocatedRack->
                        compositions[wload.allocation.composition].assignedWorkloads.end();
            ++i) {
            if(*i == wloadIt) {
                found = true;
                wload.allocation.allocatedRack->compositions[wload.allocation.composition].assignedWorkloads.erase(i);
            }
        }


        //if no more workloads in comopsition, free composition
        if((--wload.allocation.allocatedRack->compositions[wload.allocation.composition].workloadsUsing)==0) {
            wload.allocation.allocatedRack->freeComposition(wload.allocation.allocatedRack, wload.allocation.composition);
            wload.allocation = {};
        } else
//            //Update other workloads times
            for(auto iw = wload.allocation.allocatedRack->compositions[wload.allocation.composition].assignedWorkloads.begin();
                iw != wload.allocation.allocatedRack->compositions[wload.allocation.composition].assignedWorkloads.end(); ++iw) {
                vector<workload>::iterator it2 = *iw;
                int newTime = this->timeDistortion(
                        wload.allocation.allocatedRack->compositions[wload.allocation.composition].numVolumes,
                        wload.allocation.allocatedRack->compositions[wload.allocation.composition].workloadsUsing);

//                cout << "FREED before: oldTime " << it2->executionTime << " new time: " << newTime << " time left: " << it2->timeLeft << endl;
                it2->timeLeft += newTime - it2->executionTime;
                it2->executionTime = newTime;
//                cout << "FREED after: " << it2->timeLeft << endl;
            }
    }
    int timeDistortion(int nRaidVolumes, int nConcurrent) {
        return ceil(this->distortion[nRaidVolumes-1].a*exp(this->distortion[nRaidVolumes-1].b*nConcurrent));
    }

//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
