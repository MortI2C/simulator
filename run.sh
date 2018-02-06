#!/bin/bash
prio=0.2
if [ $# -eq 1 ]; then
  prio=$1
fi
for i in $(seq 1 1 100); do
   ./simulator $i "layouts/layout-2.json" $prio
done
