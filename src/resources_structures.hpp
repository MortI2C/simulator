#ifndef RESOURCES_STRUCTURES_H
#define RESOURCES_STRUCTURES_H

#include <iostream>
#include <vector>
struct nvme_slot {
   int time;
   bool used;
};

struct allocated_resources {
    std::vector<nvme_slot>::iterator resource;
    int completion_time;
};

struct workload {
    bool highprio;
    int deadline;
    int completion_time;
    int arrival;
    int scheduled;
};

#endif
