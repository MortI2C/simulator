#include <iostream>
#include <vector>
#include <algorithm>
#include "degradationModel.hpp"
using namespace std;

DegradationModel::DegradationModel() {
    this->distortion = distortionValues({1706.29,1.08423,0.858092});
}

int DegradationModel::timeDistortion(int nRaidVolumes, int nConcurrent) {
    return ceil(this->distortion.a*pow(this->distortion.b,nConcurrent)*pow(this->distortion.c,nRaidVolumes));
}