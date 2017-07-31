#include <iostream>
#include <algorithm>
#include <random>
#include <vector>
#include <queue>
#include "SharedPolicy.hpp"

#ifndef NR_SHARED
#define NR_SHARED 4
#endif
#ifndef EXECUTION_TIME
#define EXECUTION_TIME 1499
#endif


using namespace std;

vector<workload> generate_arrivals(int npatients, double prio_threshold, int nconcurrent = 5) {
    vector <workload> arrivals(npatients);
    default_random_engine generator;
    uniform_real_distribution<double> distribution(0.0, 1.0);
    for (int i = 0; i < npatients; ++i) {
        arrivals[i].arrival = EXECUTION_TIME*1.5*(i / nconcurrent);
        double number = distribution(generator);
        if (number <= prio_threshold) {
            double completion = EXECUTION_TIME * 1.25 + (double)arrivals[i].arrival;
            arrivals[i].highprio = true;
            arrivals[i].deadline = (int) completion;
        } else {
            double completion = EXECUTION_TIME * 1.65 + arrivals[i].arrival;
            arrivals[i].highprio = false;
            arrivals[i].deadline = (int) completion;
        }
    }

    return arrivals;
}

int main(int argc, char* argv[]) {
    int patients=atoi(argv[1]);
    double prio_threshold = 0.2;
    if(argc>2)
        prio_threshold=atof(argv[2]);

    int step = 0;
    int processedPatients = 0;
    vector<workload> workloads = generate_arrivals(patients, prio_threshold, 5);

    SharedPolicy* policy = new SharedPolicy();
    policy->initializeResources(2);

    vector<workload>::iterator wlpointer = workloads.begin();
    while(wlpointer != workloads.end()) {
        policy->freeResources(step);
        while(processedPatients <= patients && wlpointer->arrival <= step && policy->scheduleWorkload(wlpointer, step)) {
            wlpointer++;
            processedPatients++;
        }
        step++;
    }

    int missedDeadline = 0;
    for(vector<workload>::iterator it = workloads.begin(); it != workloads.end(); it++) {
        if(it->completion_time > step) step = it->completion_time;
        if(it->deadline < it->completion_time) ++missedDeadline;
    }

//    cout << step << endl;
    cout << missedDeadline << endl;

    return 0;
}
