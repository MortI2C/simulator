#!/bin/bash
#echo -e "Lambda\tFCFS-FirstFit\tFCFS-OverProvisioning\tFCFS-MinFrag\tEDF-FirstFit\tEDF-OverProvisioning\tEDF-MinFrag\tGroupedEDF-FirstFit\tGroupedEDF-OverProvisioning\tGroupedEDF-MinFrag\tEDFw/BF-FirstFit\tEDFw/BF-OverProvisioning\tEDFw/BF-MinFrag\tGroupedEDFw/BF-FirstFit\tGroupedEDFw/BF-OverProvisioning\tGropuedEDFw/BF-MinFrag"
for p in $(seq 0.7 0.1 1.9); do
   lambda=$(echo $p | tr "," ".") 
   ./simulator 1500 "layouts/layout-8.json" $lambda 0.2 1.2 > logs/logqos_io-intensive-${lambda}lambda.txt
done
