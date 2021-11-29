#!/bin/bash
for p in $(seq 1 0.1 1.4); do
   prio=$(echo $p | tr "," ".")
   ./simulator 1500 "layouts/layout-8.json" 2.5 0.2 $prio
done
