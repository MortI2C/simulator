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
    std::mt19937 generator(rand_dev());
    std::uniform_int_distribution<int> distr(range_from, range_to);
    uniform_real_distribution<double> distribution(0.0, 1.0);

    for (vector<workload>::iterator it = workloads.begin(); it!=workloads.end(); ++it) {
        it->arrival = distr(generator);
//        cout << arrivals[i].arrival << " ";
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

//vector<workload> ArrivalUniformModel::generate_arrivals(int npatients, double prio_threshold, int nconcurrent = 5) {
//    //Original: 1.5, 1.25, 1.85
//    //QoS Better: 1.2, 1.06, 1.25
//    vector <workload> arrivals(npatients);
//    default_random_engine generator;
//    uniform_real_distribution<double> distribution(0.0, 1.0);
//    for (int i = 0; i < npatients; ++i) {
//        arrivals[i].arrival = this->baseExecutionTime*1*(i / nconcurrent);
//        double number = distribution(generator);
//        if (number <= prio_threshold) {
//            double completion = this->baseExecutionTime * 2 + (double)arrivals[i].arrival;
//            arrivals[i].highprio = true;
//            arrivals[i].deadline = (int) completion;
//        } else {
//            double completion = this->baseExecutionTime * 2.5 + arrivals[i].arrival; //1.5, 1.25, 1.85
//            arrivals[i].highprio = false;
//            arrivals[i].deadline = (int) completion;
//        }
//    }
//    return arrivals;
//}
