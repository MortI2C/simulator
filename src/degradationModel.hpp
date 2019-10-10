#ifndef DEGRADATION_MODEL_H
#define DEGRADATION_MODEL_H
#include <iostream>
#include <vector>
using namespace std;

class DegradationModel {
   public:
    struct distortionValues {
        float a;
        float b;
        float c;
    };

    distortionValues distortion;
    DegradationModel() {};
    int timeDistortion(int, int, double, int, int);
    int smufinModel(int, int);
};

#endif
