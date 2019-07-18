#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include "degradationModel.hpp"
using namespace std;

int DegradationModel::timeDistortion(int availableBandwidth, int baseExecutionTime, double performanceMultiplier, int baseBandwidth = 2000) {
    double extraBandwidth = availableBandwidth/baseBandwidth;
    double multiplier = performanceMultiplier*extraBandwidth;

    return ceil(baseExecutionTime*multiplier);
}
