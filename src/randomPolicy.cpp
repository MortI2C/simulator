#include <iostream>
#include <algorithm>
#include <random>
#include <vector>
#include "layout.hpp"
#include "resources_structures.hpp"
#include "randomPolicy.hpp"
#include "placementPolicy.hpp"
using namespace std;

bool RandomScheduler::scheduleWorkloads(vector<workload> &workloads,
                                        vector<int> &pendingToSchedule,
                                        vector<int> &runningWorkloads,
                                        PlacementPolicy *placementPolicy, int step, Layout &layout)
{

    // CHANGE FROM HERE

    vector<int> toFinish;
    int size = pendingToSchedule.size();

    std::random_device rand_dev;
    std::mt19937 generator(5);
    std::uniform_int_distribution<int> distribution(0, size - 1);

    //Determine start state (according to state probabilities)

    for (int i = 0; i < size; i++)
    {

        int job = pendingToSchedule[distribution(generator)];
        // cout << "i = " << i << " job=" << job << " step=" << step  << endl;
        if (workloads[job].scheduled == step)
        {
            // cout << "continue"<< endl;
            continue;
        }
       
        // If missed deadline, do not starve further. Run right away (-1 deadline)
        // int deadline = (step > workloads[*it].deadline) ? -1 : workloads[*it].deadline;

        if (placementPolicy->placeWorkload(workloads, job, layout, step, workloads[job].deadline))
        {
            
            //cout << "BEFORE: load factor " << layout.loadFactor(workloads,pendingToSchedule,runningWorkloads) << endl;
            workloads[job].scheduled = step;
            runningWorkloads.push_back(job);
            toFinish.push_back(job);
            //cout << "AFTER: load factor " << layout.loadFactor(workloads,pendingToSchedule,runningWorkloads) << endl;
            this->log(job, workloads, pendingToSchedule, runningWorkloads, placementPolicy, step, layout);
        }
    }

    // DONT GO BEYOND THIS POINT
    //Remove already placed workloads
    for (auto it = toFinish.begin(); it != toFinish.end(); ++it)
    {
        for (auto it2 = pendingToSchedule.begin(); it2 != pendingToSchedule.end(); ++it2)
        {
            if (*it2 == *it)
            {
                pendingToSchedule.erase(it2);
                break;
            }
        }
    }
}