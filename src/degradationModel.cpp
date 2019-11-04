#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
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
    return ceil(this->distortion.a*totalBandwidth+this->distortion.b*totalRuns+this->distortion.c);
}

