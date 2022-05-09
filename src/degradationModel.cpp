#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <assert.h>
#include "degradationModel.hpp"
#include "resources_structures.hpp"
using namespace std;

DegradationModel::DegradationModel() {
//    this->distortion = distortionValues({1706.29,1.08423,0.858092});
//    this->distortion = distortionValues({-0.021875,97.9734,1276.36}); SKX mix numbers
    this->distortion = distortionValues({-0.113236,123.627,1719.01});
}

int DegradationModel::timeDistortion(raid& composition, workload& wload) {
    if(wload.wlName == "smufin") {
        assert(composition.composedNvme.getTotalBandwidth()>=2000);
        return this->smufinModel(composition.composedNvme.getTotalBandwidth(), composition.workloadsUsing);
    }

    if(wload.nvmeBandwidth == 0)
        return wload.executionTime;

    int availableBandwidth = composition.composedNvme.getAvailableBandwidth();
    double extraBandwidth = (availableBandwidth/wload.baseBandwidth) - 1;
    if(availableBandwidth>wload.limitPeakBandwidth)
        extraBandwidth = (wload.limitPeakBandwidth/wload.baseBandwidth) - 1;
    double multiplier = pow(wload.performanceMultiplier,extraBandwidth);

    return ceil(wload.executionTime*multiplier);
}

int DegradationModel::smufinModel(int totalBandwidth, int totalRuns) {
    int bwModel = ceil(totalBandwidth/2000);
    if(bwModel > 3) bwModel = 3;
    if(totalRuns<=6 && bwModel>0 && totalRuns>0) {
        return this->values[bwModel - 1][totalRuns - 1];
    } else {
        return ceil(this->distortion.a * totalBandwidth + this->distortion.b * totalRuns + this->distortion.c);
    }
//    return ceil(-0.116413*totalBandwidth+1.73256*exp(totalRuns)+1988.05);
}

int DegradationModel::fioModel(int totalBandwidth, int totalRuns) {
    int bwModel = ceil(totalBandwidth/2000);
    return 900/pow(2,bwModel);
}

int DegradationModel::yoloModel(int totalRuns) {
    int times[] = {151,175,282,750};
    assert(totalRuns<=4);
    return times[totalRuns-1];
}


