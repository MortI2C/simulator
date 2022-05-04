#ifndef DEGRADATION_MODEL_H
#define DEGRADATION_MODEL_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class DegradationModel {
   public:
    struct distortionValues {
        float a;
        float b;
        float c;
    };
    double values[3][6] = {
            {1489.15, 1601.25, 1677.35, 1936.245, 2411.56, 2802.62},
            {1455.48,1455.45,1474.12,1569.89,1668.35,1835.62},
//            {1337.67,1461.500000,1513.000000,1550.500000,1578.500000,1618.500000}
            {1478.500000,1461.500000,1513.000000,1550.500000,1578.500000,1618.500000}
    };

    distortionValues distortion;
    DegradationModel();
    int timeDistortion(raid&, workload&);
    int smufinModel(int, int);
    int fioModel(int,int);
    int yoloModel(int);
};

#endif
