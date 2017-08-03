#include <iostream>
#include <vector>
#include <queue>
#include "resources_structures.hpp"
#include "SharedPolicy.hpp"
#include "qosDedicated.hpp"
#include "dedicatedPolicy.hpp"
#include "arrivalUniformModel.hpp"

#ifndef NR_SHARED
#define NR_SHARED 4
#endif
#ifndef EXECUTION_TIME
#define EXECUTION_TIME 1499
#endif


using namespace std;

int calculateMissedDeadlines(Policy* policy, vector<workload>& workloads, int patients) {
    int step = 0;
    int processedPatients = 0;

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

    return missedDeadline;
}

int main(int argc, char* argv[]) {
    int patients=atoi(argv[1]);
    double prio_threshold = 0.2;
    if(argc>2)
        prio_threshold=atof(argv[2]);

    ArrivalUniformModel* arrival = new ArrivalUniformModel(EXECUTION_TIME);
    vector<workload> workloads = arrival->generate_arrivals(patients, prio_threshold, 5);

    SharedPolicy* sharedPolicy = new SharedPolicy();
    sharedPolicy->initializeResources(2);
    SharedPolicy* oneNvme = new SharedPolicy();
    oneNvme->initializeResources(1);
    DedicatedPolicy* dedicatedPolicy = new DedicatedPolicy();
    dedicatedPolicy->initializeResources();
    QoSPolicy* qosPolicy = new QoSPolicy();
    qosPolicy->initializeResources();

//    cout << step << endl;
    cout << calculateMissedDeadlines(oneNvme, workloads, patients) << " " <<
         calculateMissedDeadlines(dedicatedPolicy, workloads, patients) << " " <<
         calculateMissedDeadlines(qosPolicy, workloads, patients) << " " <<
         calculateMissedDeadlines(sharedPolicy, workloads, patients) << endl;

    return 0;
}
