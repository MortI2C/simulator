#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "degradationModel.hpp"
using namespace std;

DegradationModel::DegradationModel() {
//    this->distortion = distortionValues({1706.29,1.08423,0.858092});
    this->distortion = distortionValues({-0.021875,97.9734,1276.36});
}

int DegradationModel::timeDistortion(int availableBandwidth, int baseExecutionTime, double performanceMultiplier, int baseBandwidth, int limitPeakBandwidth) {
    double extraBandwidth = (availableBandwidth/baseBandwidth) - 1;
    if(availableBandwidth>limitPeakBandwidth)
        extraBandwidth = (limitPeakBandwidth/baseBandwidth) - 1;
    double multiplier = pow(performanceMultiplier,extraBandwidth);

    return ceil(baseExecutionTime*multiplier);
}

int DegradationModel::smufinModel(int totalBandwidth, int totalRuns) {
    return ceil(this->distortion.a*totalBandwidth+this->distortion.b*totalRuns+this->distortion.c);
}
