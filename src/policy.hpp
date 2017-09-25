#ifndef POLICY_H
#define POLICY_H
#include <iostream>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
using namespace std;

class Policy {
   public:
    virtual bool scheduleWorkload(vector<workload>::iterator, int, Layout&) =0;
//    virtual void freeResources(vector<workload>::iterator) =0;
};

#endif
