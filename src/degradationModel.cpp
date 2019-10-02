#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "degradationModel.hpp"
using namespace std;

int DegradationModel::timeDistortion(int availableBandwidth, int baseExecutionTime, double performanceMultiplier, int baseBandwidth, int limitPeakBandwidth) {
    double extraBandwidth = (availableBandwidth/baseBandwidth) - 1;
    if(availableBandwidth>limitPeakBandwidth)
        extraBandwidth = (limitPeakBandwidth/baseBandwidth) - 1;
    double multiplier = pow(performanceMultiplier,extraBandwidth);

    return ceil(baseExecutionTime*multiplier);
}
