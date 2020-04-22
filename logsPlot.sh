#!/bin/bash

for f in logs/*; do
    logDir=$(echo $f | cut -d/ -f2 | cut -d. -f1,2)
    [[ ! -d "logs_plots/$logDir" ]] && mkdir logs_plots/$logDir
    python plots/sched-plot.py -s $f -l layouts/layout-8.json
    mv plots/*.pdf logs_plots/$logDir
done
