#!/bin/bash

for i in 0.1 0.5 0.9; do
  prio=$(echo $i | tr "," ".")
  for coef in 0.7 0.8 0.9 1 2 3 4 5 6 7 8 9 10 11 12 13 14; do
    for starv in $(seq 4 1 4); do
#   for starv in $(seq 1 1 14); do
    coef=$(echo $coef | tr "," ".")
    output=$(./simulator 200 "layouts/layout-5.json" $prio $coef $starv)
    deadlines=$(echo $output | awk '{print $6}')
    deadlines=$(echo "($deadlines / 200)*100" | bc -l )
    loadFactor=$(echo $output | awk '{print $1}')
    waitTime=$(echo $output | awk '{print $3}')
    makespan=$(echo $output | awk '{print $2}')
    echo -e "$prio\t$coef\t$loadFactor\t$deadlines\t$waitTime\t$makespan\t$starv"
   done
  done
done
