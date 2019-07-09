#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "degradationModel.hpp"
using namespace std;

DegradationModel::DegradationModel() {
//    this->distortion = distortionValues({1706.29,1.08423,0.858092});
    this->distortion = distortionValues({0.210192,-0.025247,1472.47});
}

int DegradationModel::timeDistortion(int totalBandwidth, int bandwidthRequested) {
//    return ceil(this->distortion.a*pow(this->distortion.b,nConcurrent)*pow(this->distortion.c,nRaidVolumes));
    return ceil(this->distortion.a*bandwidthRequested+this->distortion.b*totalBandwidth+this->distortion.c);
}
