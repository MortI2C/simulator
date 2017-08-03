#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include "arrivalUniformModel.hpp"
using namespace std;

vector<workload> ArrivalUniformModel::generate_arrivals(int npatients, double prio_threshold, int nconcurrent = 5) {
    //Original: 1.5, 1.25, 1.85
    //QoS Better: 1.2, 1.06, 1.25
    vector <workload> arrivals(npatients);
    default_random_engine generator;
    uniform_real_distribution<double> distribution(0.0, 1.0);
    for (int i = 0; i < npatients; ++i) {
        arrivals[i].arrival = this->baseExecutionTime*1.2*(i / nconcurrent);
        double number = distribution(generator);
        if (number <= prio_threshold) {
            double completion = this->baseExecutionTime * 1.06 + (double)arrivals[i].arrival;
            arrivals[i].highprio = true;
            arrivals[i].deadline = (int) completion;
        } else {
            double completion = this->baseExecutionTime * 1.25 + arrivals[i].arrival; //1.5, 1.25, 1.85
            arrivals[i].highprio = false;
            arrivals[i].deadline = (int) completion;
        }
    }
    return arrivals;
}
//
//vector<workload> ArrivalUniformModel::generate_arrivals(int npatients, double prio_threshold, int nconcurrent = 5) {
//    vector <workload> arrivals(npatients);
//    default_random_engine generator;
//    uniform_real_distribution<double> distribution(0.0, 1.0);
//    for (int i = 0; i < npatients; ++i) {
//        arrivals[i].arrival = this->baseExecutionTime*1.5*(i / nconcurrent);
//        double number = distribution(generator);
//        if (number <= prio_threshold) {
//            double completion = this->baseExecutionTime * 1.25 + (double)arrivals[i].arrival;
//            arrivals[i].highprio = true;
//            arrivals[i].deadline = (int) completion;
//        } else {
//            double completion = this->baseExecutionTime * 1.65 + arrivals[i].arrival;
//            arrivals[i].highprio = false;
//            arrivals[i].deadline = (int) completion;
//        }
//    }
//
//    return arrivals;
//}
