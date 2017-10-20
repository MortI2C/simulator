#ifndef PLACEMENT_POLICY_H
#define PLACEMENT_POLICY_H
#include <iostream>
#include <vector>
#include <algorithm>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "nvmeResource.hpp"
using namespace std;

class PlacementPolicy {
   public:
    struct distortionValues {
        float a;
        float b;
        float c;
    };
    distortionValues distortion;
    double loadFactor;
//    vector<distortionValues> distortion;
    PlacementPolicy() {
//        this->distortion = vector<distortionValues>(2);
//          this->distortion = distortionValues({919.078,-76.67,1.58321});
        this->distortion = distortionValues({1706.29,1.08423,0.858092});
//        this->distortion[0].a = 1181.22;
//        this->distortion[0].b = 0.14008;
//        this->distortion[1].a = 1322.88;
//        this->distortion[1].b = 0.490824;
    }
    virtual bool placeWorkload(vector<workload>&, int, Layout&, int) =0;
    void freeResources(vector<workload>& workloads, int wloadIt) {
        workload* wload = &workloads[wloadIt];

        wload->allocation.allocatedRack->compositions
        [wload->allocation.composition].composedNvme
                .setAvailableBandwidth(wload->allocation.allocatedRack->compositions
                                       [wload->allocation.composition].composedNvme
                                               .getAvailableBandwidth()+wload->nvmeBandwidth);

        wload->allocation.allocatedRack->compositions
        [wload->allocation.composition].composedNvme
                .setAvailableCapacity(wload->allocation.allocatedRack->compositions
                                      [wload->allocation.composition].composedNvme
                                              .getAvailableCapacity()+wload->nvmeCapacity);

        //Remove Workload from assigned compositions Wloads
        bool found = false;
        for(auto i = wload->allocation.allocatedRack->
                compositions[wload->allocation.composition].assignedWorkloads.begin();
                !found && i!=wload->allocation.allocatedRack->
                        compositions[wload->allocation.composition].assignedWorkloads.end();
            ++i) {
            workload ptWload = workloads[*i];
            if(ptWload.wlId == wload->wlId) {
                found = true;
                wload->allocation.allocatedRack->compositions[wload->allocation.composition].assignedWorkloads.erase(i);
            }
        }

        //if no more workloads in comopsition, free composition
        if((--wload->allocation.allocatedRack->compositions[wload->allocation.composition].workloadsUsing)==0) {
            wload->allocation.allocatedRack->freeComposition(wload->allocation.allocatedRack, wload->allocation.composition);
            wload->allocation = {};
        } else {
            //Update other workloads times
            for (auto iw = wload->allocation.allocatedRack->compositions[wload->allocation.composition].assignedWorkloads.begin();
                 iw !=
                 wload->allocation.allocatedRack->compositions[wload->allocation.composition].assignedWorkloads.end(); ++iw) {

                workload it2 = workloads[*iw];
                int newTime = this->timeDistortion(
                        wload->allocation.allocatedRack->compositions[wload->allocation.composition].numVolumes,
                        wload->allocation.allocatedRack->compositions[wload->allocation.composition].workloadsUsing);
                it2.timeLeft = ((float) it2.timeLeft / it2.executionTime) * newTime;
                it2.executionTime = newTime;
            }
        }
    }
    int timeDistortion(int nRaidVolumes, int nConcurrent) {
        return ceil(this->distortion.a*pow(this->distortion.b,nConcurrent)*pow(this->distortion.c,nRaidVolumes));
//      return ceil(this->distortion.a*exp(this->distortion.b*nConcurrent)+this->distortion.c*exp(this->distortion.d*nRaidVolumes));
//        return ceil(this->distortion[nRaidVolumes-1].a*exp(this->distortion[nRaidVolumes-1].b*nConcurrent));
    }

//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
