# calculate the spearman's correlation between two variables
from numpy.random import rand
from numpy.random import seed
from scipy.stats import spearmanr
import json

data1 = {}
data2 = {}
array_fcfs = []
array_earliest = []
with open('earliest_output1500.txt') as jsonfile:
    data1  = json.load(jsonfile)

with open('fcfs_output1500.txt') as jsonfile:
    data2 = json.load(jsonfile)

schedule = sorted(data1, key=lambda k: k['step'], reverse=False)

for i in schedule:
    array_fcfs.append(i["jobid"][0])

schedule = sorted(data2, key=lambda k: k['step'], reverse=False)

for i in schedule:
    array_earliest.append(i["jobid"][0])

# prepare data
# calculate spearman's correlation
coef, p = spearmanr(array_fcfs, array_earliest)
print('Spearmans correlation coefficient: %.3f' % coef)
# interpret the significance
alpha = 0.05
if p > alpha:
	print('Samples are uncorrelated (fail to reject H0) p=%.3f' % p)
else:
	print('Samples are correlated (reject H0) p=%.3f' % p)
