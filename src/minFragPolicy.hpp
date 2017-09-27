#ifndef MIN_FRAG_POLICY_H
#define MIN_FRAG_POLICY_H
#include <iostream>
#include <vector>
#include "policy.hpp"
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class MinFragPolicy : public Policy {
   public:
    void insertSorted(vector<nvmeFitness>&, nvmeFitness);
    bool scheduleWorkload(vector<workload>::iterator, int, Layout&);
};

#endif
