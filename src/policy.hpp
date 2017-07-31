#ifndef POLICY_H
#define POLICY_H
#include <iostream>
#include <vector>
#include "resources_structures.hpp"
using namespace std;

class Policy {
   public:
    virtual bool scheduleWorkload(vector<workload>::iterator, int) =0;
};

#endif
