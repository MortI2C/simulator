#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip>
#include "util.cpp"
#include "math.h"
#include "arrivalPoissonModel.hpp"
using namespace std;

struct by_arrival {
    bool operator()(workload const &a, workload const &b) const {
        return a.arrival < b.arrival;
    }
};

void ArrivalPoissonModel::generate_arrivals(vector<workload>& workloads, float timeInterval, double prio_threshold) {
    //Original: 1.5, 1.25, 1.85
    //QoS Better: 1.2, 1.06, 1.25
    const int range_from = 0;
    const float range_to = timeInterval;
    std::random_device rand_dev;
    std::mt19937 generator(5);
    uniform_real_distribution<double> distribution(0.0, 1.0);
    //lambda(t) = 3.1 - 8.5 t + 24.7 t2 + 130.8 t3 + 107.7 t4 - 804.2 t5 - 2038.5 t6 + 1856.8 t7 + 4618.6 t8

    for(vector<workload>::iterator it = workloads.begin(); it!=workloads.end(); ++it) {
        it->arrival = (int)nextTime(1/timeInterval);
//        cout << it->arrival << " ";
        double number = distribution(generator);
        if (number <= prio_threshold) {
            double completion = it->executionTime * 1.25 + it->arrival;
            it->highprio = true;
            it->deadline = (int) completion;
        } else {
            double completion = it->executionTime * 1.85 + it->arrival; //1.5, 1.25, 1.85
            it->highprio = false;
            it->deadline = (int) completion;
        }
    }
//    cout << endl;
    sort(workloads.begin(), workloads.end(), by_arrival());
    //Make sure unique arrival values and make wlid = index after sorting
    int i = 0;
    for(auto it = workloads.begin(); it!=workloads.end(); ++it,++i) {
//        if((it+1)!=workloads.end() && it->arrival == (it+1)->arrival) {
//            (it+1)->arrival++;
//        }
        it->wlId = i;
    }
//
//    for(auto it = workloads.begin(); it!=workloads.end(); ++it) {
//        cout << it->arrival << " ";
//    }
//    cout << endl;
}
