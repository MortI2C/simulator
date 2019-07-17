#ifndef MARKOVMODULATEPOISSON2STATE_MODEL_UNIFORM_H
#define MARKOVMODULATEPOISSON2STATE_MODEL_UNIFORM_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class MarkovModulatedpoissonProcess2 {
   private:
    //Get parameters
    double lambda[2];
    double omega[2];
    double timeToArrival;
    double timeToTransition;
    double timeToIntervalEnd;
   public:
    MarkovModulatedpoissonProcess2(double lambda1 = 10, double lambda2 = 1, double omega1 = 0.1, double omega2 = 0.1) {
        this->lambda[0] = lambda1;
        this->lambda[1] = lambda2;
        this->omega[0] = omega1;
        this->omega[1] = omega2;
    }
    void generate_arrivals(vector<workload>&, int, double);
    void adjustTimes(double);
};

#endif
