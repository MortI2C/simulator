#ifndef DEGRADATION_MODEL_H
#define DEGRADATION_MODEL_H
#include <iostream>
#include <vector>
using namespace std;

class DegradationModel {
   public:

    DegradationModel() {};
    int timeDistortion(int, int, double, int, int);
};

#endif
