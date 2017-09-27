#ifndef BEST_FIT_POLICY_H
#define BEST_FIT_POLICY_H
#include <iostream>
#include <vector>
#include "policy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class BestFitPolicy : public Policy {
   public:
    void insertSorted(vector<nvmeFitness>&, nvmeFitness);
    bool scheduleWorkload(vector<workload>::iterator, int, Layout&);
};

#endif