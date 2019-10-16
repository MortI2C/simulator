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

    distortionValues distortion;
    DegradationModel();
    int timeDistortion(raid&, workload&);
    int smufinModel(int, int);
};

#endif
