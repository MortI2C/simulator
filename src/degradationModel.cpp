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
    if(wload.wlName == "smufin")
        return this->smufinModel(composition.composedNvme.getAvailableBandwidth(), composition.workloadsUsing);

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

