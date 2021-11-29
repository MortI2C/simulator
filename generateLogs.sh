#!/bin/bash
#echo -e "Lambda\tFCFS-FirstFit\tFCFS-OverProvisioning\tFCFS-MinFrag\tEDF-FirstFit\tEDF-OverProvisioning\tEDF-MinFrag\tGroupedEDF-FirstFit\tGroupedEDF-OverProvisioning\tGroupedEDF-MinFrag\tEDFw/BF-FirstFit\tEDFw/BF-OverProvisioning\tEDFw/BF-MinFrag\tGroupedEDFw/BF-FirstFit\tGroupedEDFw/BF-OverProvisioning\tGropuedEDFw/BF-MinFrag"
for p in 0.8 1.6 2.0 2.4 2.9 3.4; do #IO-intensive
#for p in 1.0 1.3 1.5 1.8 2.0; do #storage-based
#for p in 1.8 2.1 2.5 3.0 3.4 4.0; do #compute-intensive
   lambda=$(echo $p | tr "," ".") 
   ./simulator 1500 "layouts/layout-8.json" $lambda 0.2 1.2 > logs/logfirstfit_io-intensive-${lambda}lambda.txt
   ./simulator 1500 "layouts/layout-7.json" $lambda 0.2 1.2 > logs/PAS_logfirstfit_io-intensive-${lambda}lambda.txt
done
