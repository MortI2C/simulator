#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# sched-plot -- Plot a workload schedule
#
# Given a system configuration, a workload description, and a schedule as
# generated by `scorsa-sched', this program generates a plot displaying the
# amount of sockets used by each job over time, as well as submission times
# for each job in the workload.
#
# Copyright © 2018 Aaron Call <aaron.callo@bsc.es>

import sys
import logging
import argparse
import json
import copy
import math

import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt

logging.basicConfig(format="%(message)s", level=logging.ERROR)

ap = argparse.ArgumentParser()
ap.add_argument("-l", "--layout", dest="l", required=True,
               help="Layout file (json format)")
ap.add_argument("-s", "--schedule", dest="s", required=True,
				help="JSON schedule file")
args = ap.parse_args()

with open(args.s) as s:
	schedule = json.load(s)

with open(args.l) as l:
    layout = json.load(l)

totalCores = 0
for rack in layout:
    totalCores += layout[rack]["cores"]

lastTime = sorted(schedule, key=lambda k: k['completion'])[-1]["completion"]
schedule = sorted(schedule, key=lambda k: k['step'], reverse=False)

loadFactor = []
responseTime = []
arrivalsPlot = []
resources = []
labels = []
completions = {}
parsedSched = {}
racks = {}
resourcesUsed = 0
arrivals = 0
arrivalJson = {}
avgCompositionPlot = []
missedDeadlines = []
avgCompSize = 0
avgWorkloadsSharing = 0
avgWorkloadsSharingPlot = []
arrivalAccPlot = []
departureAccPlot = []
arrivalAccJson = {}
departureAccJson = {}
departureAccPlot = []
arrivalsAcc = 0
departuresAcc = 0
lf = 0
deadlines = 0
interArrivalTimes = 0
previousTime = -1
arrivalsJson = {}
bwJson = {}
capJson = {}
cpuJson = {}
jobs = 0
jobsPlot = []
labelsArrivals = []
wlsPercentagesBW = []
wlsPercentagesCap = []
wlsPercentagesCPU = []
wlsPercentagesLabels = []
wlsCoresBW = []
wlsCoresCap = []
wlsCoresCPU = []
abstractLoadFactor = []
absLfCPU = []
absLfBW = []
absLfCap = []
labelsLf = []

def add_wload(wlName, timeStamp):
    if wlName == "tpcxiot":
        if timeStamp not in capJson:
            capJson[timeStamp] = 1
        else:
            capJson[timeStamp] += 1
    elif wlName == "smufin":
        if timeStamp not in bwJson:
            bwJson[timeStamp] = 1
        else:
            bwJson[timeStamp] += 1
    else:
        if timeStamp not in cpuJson:
            cpuJson[timeStamp] = 1
        else:
            cpuJson[timeStamp] += 1

def delete_wload(wlName, timeStamp):
    if wlName == "tpcxiot":
        if timeStamp not in capJson:
            capJson[timeStamp] = -1
        else:
            capJson[timeStamp] -= 1
    elif wlName == "smufin":
        if timeStamp not in bwJson:
            bwJson[timeStamp] = -1
        else:
            bwJson[timeStamp] -= 1
    else:
        if timeStamp not in cpuJson:
            cpuJson[timeStamp] = -1
        else:
            cpuJson[timeStamp] -= 1

for i in schedule:
        add_wload(i["wlName"][0],i["scheduled"][0])
        delete_wload(i["wlName"][0],i["completion"])
        arrivalsJson[i["arrival"][0]] = i
        if i["scheduled"][0] in parsedSched:
            parsedSched[i["scheduled"][0]].append(i)
        else:
            parsedSched[i["scheduled"][0]] = [i]

        if i["arrival"][0] not in arrivalJson:
            arrivalJson[i["arrival"][0]] = 1
        else:
            arrivalJson[i["arrival"][0]] += 1

        if i["arrival"][0] not in arrivalAccJson:
            arrivalAccJson[i["arrival"][0]] = 1
        else:
            arrivalAccJson[i["arrival"][0]] += 1

        if i["completion"] not in departureAccJson:
            departureAccJson[i["completion"]] = 1
        else:
            departureAccJson[i["completion"]] += 1

        responseTime.append(i["scheduled"][0] - i["arrival"][0])

#        if i["completion"] in arrivalJson:
#            arrivalJson[i["completion"]] -= 1
#        else:
#            arrivalJson[i["completion"]] = -1

wlBw = 0
wlCap = 0
wlCpu = 0
for i in range(0, lastTime+1):
        lf = -1
        abstractLf = -1
        lfCPU = -1
        lfBW = -1
        lfCap = -1
        if i in completions:
            for p in completions[i]:
                jobs -= 1
                if p["deadline"][0] < p["completion"]:
                    deadlines += 1
                for volume in p["volumes"]:
                    racks[p["rackid"][0]][volume] -= 1
                if racks[p["rackid"][0]][volume] == 0:
                    resourcesUsed -= 1
                    del racks[p["rackid"][0]][volume]

        if i in arrivalsJson:
            if previousTime == -1:
                interArrivalTimes+=i
                previousTime=i
            else:
                interArrivalTimes+=(i-previousTime)
                previousTime=i

        if i in parsedSched:
            for job in parsedSched[i]:
                jobs += 1
                avgCompSize = job["avgCompositionSize"]
                avgWorkloadsSharing = job["avgWorkloadsSharing"] 
                if job["completion"] in completions:
		            completions[job["completion"]].append(job)
                else:
                    completions[job["completion"]] = [job]

                lf = job["loadFactor"]
                abstractLf = job["abstractLoadFactorCompletion"]
                lfBW = job["idealLFBW"]
                lfCap = job["idealLFCap"]
                lfCPU = job["idealLFCPU"]
		for volume in job["volumes"]:
		    if job["rackid"][0] not in racks:
		        racks[job["rackid"][0]] = {}

		    if volume not in racks[job["rackid"][0]]:
		        racks[job["rackid"][0]][volume] = 1
		        resourcesUsed+=1
		    else:
		        racks[job["rackid"][0]][volume]+=1

        if i in bwJson:
            wlBw += bwJson[i]
        if i in capJson:
            wlCap += capJson[i]
        if i in cpuJson:
            wlCpu += cpuJson[i]

        totalWl = wlBw+wlCap+wlCpu
        wlsPercentagesLabels.extend([i])
        check = 2*wlBw + 10*wlCap + 15*wlCpu
        # if check > totalCores:
        #     print(str(wlBw)+" "+str(wlCap)+" "+str(wlCpu)+" "+str(check))
        if totalWl > 0:
            wlsCoresBW.extend([100*(6*float(wlBw))/float(totalCores)])
            wlsCoresCap.extend([100*(10*float(wlCap))/float(totalCores)])
            wlsCoresCPU.extend([100*(15*float(wlCpu))/float(totalCores)])
            wlsPercentagesBW.extend([100*float(wlBw) / float(totalWl)])
            wlsPercentagesCap.extend([100*float(wlCap) / float(totalWl)])
            wlsPercentagesCPU.extend([100*float(wlCpu) / float(totalWl)])
        else:
            wlsCoresBW.extend([0])
            wlsCoresCap.extend([0])
            wlsCoresCPU.extend([0])
            wlsPercentagesBW.extend([0])
            wlsPercentagesCap.extend([0])
            wlsPercentagesCPU.extend([0])

        # print("{} {} {}".format(wlsPercentagesBW[i],wlsPercentagesCap[i],wlsPercentagesCPU[i]))
        if i in arrivalJson:
        #    arrivals+=arrivalJson[i]
            arrivalsPlot.extend([arrivalJson[i]])
            labelsArrivals.extend([i])
#        else:
#            arrivalsPlot.extend([0])
       
        if i in arrivalAccJson:
            arrivalsAcc+=arrivalAccJson[i]

        if i in departureAccJson:
            departuresAcc+=departureAccJson[i]

        labels.extend([i])
        resources.extend([resourcesUsed])
       # arrivalsPlot.extend([arrivals])
        arrivalAccPlot.extend([arrivalsAcc])
        departureAccPlot.extend([departuresAcc])
        avgCompositionPlot.extend([avgCompSize])
        avgWorkloadsSharingPlot.extend([avgWorkloadsSharing])
        missedDeadlines.extend([deadlines])
        jobsPlot.extend([jobs])
        if lf != -1:
            labelsLf.extend([i])
            loadFactor.append(round(lf,1))
            abstractLoadFactor.append(round(abstractLf,1))
            absLfCPU.append(round(lfCPU,1))
            absLfBW.append(round(lfBW,1))
            absLfCap.append(round(lfCap,1))

# print(missedDeadlines[-1])
#print(interArrivalTimes/len(schedule))
plt.xlabel("Execution time (s)")
# plt.title("NVMe used for job allocation over time")
# plt.xticks(range(0,lastTime),rotation=90)
# pl = plt.hist(resources, 100, density=True, facecolor='g', alpha=0.75, cumulative=True, histtype='step')
# plt.xlabel('NVMe used')
# plt.ylim(0,1)
# plt.xticks(np.arange(0, max(resources)+1, 1), rotation='vertical')
# plt.savefig('plots/nvmeUsed.pdf', bbox_inches='tight')

plt.clf()
pl = plt.plot(labelsArrivals, arrivalsPlot, '^')
plt.savefig('plots/arrivals-over-time.pdf', bbox_inches='tight')

plt.clf()
pl = plt.plot(labels, avgCompositionPlot)
plt.savefig('plots/avg-compositionSize-over-time.pdf', bbox_inches='tight')

plt.clf()
pl = plt.plot(labels, avgWorkloadsSharingPlot)
plt.savefig('plots/avg-wlSharing-over-time.pdf', bbox_inches='tight')

plt.clf()
pl = plt.hist(loadFactor, 100, density=True, facecolor='g', alpha=0.75)#,  histtype='step')
plt.ylabel('# Events')
plt.xlabel('Load factor')
#plt.ylim(0,1)
plt.xticks(np.arange(0, max(loadFactor)+1, 0.5), rotation='vertical')
plt.savefig('plots/loadFactor-over-time.pdf', bbox_inches='tight')

plt.clf()
plt.plot(labelsLf,loadFactor)
#pl = plt.hist(loadFactor, 100, density=True, facecolor='g', alpha=0.75, cumulative=1)#,  histtype='step')
#plt.ylabel('Likelihood')
#plt.xlabel('Abstract Load factor')
#plt.ylim(0,1)
#plt.xticks(np.arange(0, max(abstractLoadFactor)+1, 0.5), rotation='vertical')
plt.savefig('plots/abstractloadFactor-over-time.pdf', bbox_inches='tight')
plt.clf()
plt.xlabel("Simulation time(s)")
plt.ylabel("Load factor")
plt.plot(labelsLf,absLfCap,label='Capacity load factor')
plt.plot(labelsLf,absLfBW,label='Bandwidth load factor')
plt.plot(labelsLf,absLfCPU,label='CPU load factor')
plt.legend()
plt.savefig('plots/abslfs-over-time.pdf', bbox_inches='tight')
plt.clf()
plt.plot(wlsPercentagesLabels,wlsPercentagesBW,label='Bandwidth-intensive')
plt.plot(wlsPercentagesLabels,wlsPercentagesCap,label='Capacity-intensive')
plt.plot(wlsPercentagesLabels,wlsPercentagesCPU,label='CPU-intensive')
plt.legend()
plt.xlabel("Simulation time(s)")
plt.ylabel("Percentage of workloads kind")
plt.savefig('plots/percentages-over-time.pdf',bbox_inches='tight')

plt.clf()
plt.plot(wlsPercentagesLabels,wlsCoresBW,label='Bandwidth-intensive')
plt.plot(wlsPercentagesLabels,wlsCoresCap,label='Capacity-intensive')
plt.plot(wlsPercentagesLabels,wlsCoresCPU,label='CPU-intensive')
plt.legend()
plt.xlabel("Simulation time(s)")
plt.ylabel("Percentage of workloads kind")
plt.savefig('plots/cores-perc-over-time.pdf',bbox_inches='tight')

plt.clf()

pl = plt.plot(labels, missedDeadlines)
plt.savefig('plots/missedDeadlines-over-time.pdf', bbox_inches='tight')

plt.clf()
pl = plt.plot(labels, arrivalAccPlot)
plt.plot(labels, departureAccPlot)
plt.savefig('plots/accumulatedArrivDepChart.pdf', bbox_inches='tight')

# plt.clf()
# pl = plt.plot(labels, jobsPlot)
# plt.savefig('plots/jobsOverTime.pdf', bbox_inches='tight')

plt.clf()
pl = plt.hist(responseTime, 100, density=True, facecolor='g', alpha=0.75, cumulative=True, histtype='step')
plt.ylabel('Probability')
plt.xlabel('Response time')
plt.ylim(0,1)
plt.xticks(np.arange(0, max(responseTime)+1, 500), rotation='vertical')
plt.savefig('plots/responseTime.pdf', bbox_inches='tight')
