#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>
#include "util.cpp"
#include "math.h"
#include "mmpp-2.hpp"
using namespace std;

struct by_arrival {
    bool operator()(workload const &a, workload const &b) const {
        return a.arrival < b.arrival;
    }
};

//Update all times
void MarkovModulatedpoissonProcess2::adjustTimes( double aTime )
{
    this->timeToArrival -= aTime;
    this->timeToTransition -= aTime;
    this->timeToIntervalEnd -= aTime;
}

void MarkovModulatedpoissonProcess2::generate_arrivals(vector<workload>& workloads, int intervals, double prio_threshold) {
    //Original: 1.5, 1.25, 1.85
    //QoS Better: 1.2, 1.06, 1.25
    const int range_from = 0;

    std::random_device rand_dev;
    std::mt19937 generator(5);
    std::uniform_int_distribution<int> distribution(0,1);
    //Determine start state (according to state probabilities)
    int state = !(distribution(generator));
    //Initialize times
    std::exponential_distribution<double> lambdaDist(this->lambda[state]);
    this->timeToArrival = 1/lambdaDist(generator);
    std::exponential_distribution<double> omegaDist(this->omega[state]);
    this->timeToTransition = 1/omegaDist(generator);
    cerr << this->timeToArrival << " " << this->timeToTransition << endl;
    vector<workload>::iterator it = workloads.begin();
    int step = this->timeToArrival;
    for(int i = 0; i<workloads.size();) {
        if(this->timeToArrival < this->timeToTransition) {
            it->arrival = step;
            this->adjustTimes(this->timeToArrival);
            step+=this->timeToArrival;
            lambdaDist = std::exponential_distribution<double>(this->lambda[state]);
            this->timeToArrival = 1/lambdaDist(generator);
            ++it;
            i++;
        } else {
            this->adjustTimes(this->timeToTransition);
            state != state;
            omegaDist = std::exponential_distribution<double>(this->omega[state]);
            this->timeToTransition = 1/omegaDist(generator);
            step+=timeToTransition;
        }
    }

//    cout << endl;
    sort(workloads.begin(), workloads.end(), by_arrival());
    //Make sure unique arrival values and make wlid = index after sorting
    int i = 0;
    for(auto it = workloads.begin(); it!=workloads.end(); ++it,++i) {
        std::random_device rand_dev;
        std::mt19937 generator(5);
        uniform_real_distribution<double> distribution(0.0, 1.0);
        double number = distribution(generator);
        if (number <= prio_threshold) {
            double completion = it->executionTime * 1.05 + it->arrival;
            it->highprio = true;
            it->deadline = (int) completion;
        } else {
            double completion = it->executionTime * 1.85 + it->arrival; //1.5, 1.25, 1.85
            it->highprio = false;
            it->deadline = (int) completion;
        }
//        if((it+1)!=workloads.end() && it->arrival == (it+1)->arrival) {
//            (it+1)->arrival++;
//        }
        it->wlId = i;
//        cerr << it->arrival << " ";
    }
//    cerr << "finish" << endl;
//    cerr << endl;
}
