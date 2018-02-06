#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include "arrivalUniformModel.hpp"
using namespace std;

struct by_arrival {
    bool operator()(workload const &a, workload const &b) const {
        return a.arrival < b.arrival;
    }
};

void ArrivalUniformModel::generate_arrivals(vector<workload>& workloads, int timeInterval, double prio_threshold) {
    //Original: 1.5, 1.25, 1.85
    //QoS Better: 1.2, 1.06, 1.25

    const int range_from = 0;
    const int range_to = timeInterval;
    std::random_device rand_dev;
    std::mt19937 generator(5);
    std::uniform_int_distribution<int> distr(range_from, range_to);
    uniform_real_distribution<double> distribution(0.0, 1.0);

    for (vector<workload>::iterator it = workloads.begin(); it!=workloads.end(); ++it) {
        it->arrival = distr(generator);
//        cout << arrivals[i].arrival << " ";
        double number = distribution(generator);
        if (number <= prio_threshold) {
            double completion = it->executionTime * 1.20 + it->arrival;
            it->highprio = true;
            it->deadline = (int) completion;
        } else {
            double completion = it->executionTime * 1.80 + it->arrival;
            it->highprio = false;
            it->deadline = (int) completion;
        }
    }

    sort(workloads.begin(), workloads.end(), by_arrival());
    int i = 0;
    for(auto it = workloads.begin(); it!=workloads.end(); ++it,++i) {
        if((it+1)!=workloads.end() && it->arrival == (it+1)->arrival) {
            (it+1)->arrival++;
        }
        it->wlId = i;
    }
}