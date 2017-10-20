#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include "arrivalRegularModel.hpp"
using namespace std;

struct by_arrival {
    bool operator()(workload const &a, workload const &b) const {
        return a.arrival < b.arrival;
    }
};

void ArrivalRegularModel::generate_arrivals(vector<workload>& workloads, int arrivalRate, double prio_threshold) {
    //Original: 1.5, 1.25, 1.85
    //QoS Better: 1.2, 1.06, 1.25

//    const int range_from = 0;
//    const int range_to = timeInterval;
    std::random_device rand_dev;
    std::mt19937 generator(5);
//    std::uniform_int_distribution<int> distr(range_from, range_to);
    uniform_real_distribution<double> distribution(0.0, 1.0);

    int i = 0;
    for (vector<workload>::iterator it = workloads.begin(); it!=workloads.end(); ++it,++i) {
        it->arrival = i*arrivalRate;
//        cout << it->arrival << " ";
        double number = distribution(generator);
        if (number <= prio_threshold) {
            double completion = it->executionTime * 1.09 + it->arrival;
            it->highprio = true;
            it->deadline = (int) completion;
        } else {
            double completion = it->executionTime * 1.25 + it->arrival;
            it->highprio = false;
            it->deadline = (int) completion;
        }
    }

    sort(workloads.begin(), workloads.end(), by_arrival());
}