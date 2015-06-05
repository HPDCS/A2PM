#!/bin/python
import csv
import sys
import ast

columns = ['RTTC','gen_time','n_th_slope','mem_used_slope','mem_free_slope','mem_shared_slope','mem_buffers_slope','mem_cached_slope','swap_used_slope','swap_free_slope','cpu_user_slope','cpu_nice_slope','cpu_system_slope','cpu_iowait_slope','cpu_steal_slope','cpu_idle_slope','n_th','mem_used','mem_free','mem_shared','mem_buffers','mem_cached','swap_used','swap_free','cpu_user','cpu_nice','cpu_system','cpu_iowait','cpu_steal','cpu_idle']

if len(sys.argv) < 3:
	print "You must specify lambda_val and beta_vector"
	exit()

lambda_val = ast.literal_eval( sys.argv[1] )
beta = ast.literal_eval( sys.argv[2] )


print "REDUCING DATASET WITH WEIGHTS FROM LAMBDA " + str(lambda_val)


data = []

print "Applying weights and computing LASSO..."

#load and apply weights
with open('aggregated.csv') as csvfile:
	next(csvfile) # Skip header
	for row in csv.DictReader(csvfile, fieldnames=columns, delimiter=','):
		# Convert non-string data here
		row[columns[0]] = float(row[columns[0]])
		row[columns[1]] = beta[0] * float(row[columns[1]])
		row[columns[2]] = beta[1] * float(row[columns[2]])
		row[columns[3]] = beta[2] * float(row[columns[3]])
		row[columns[4]] = beta[3] * float(row[columns[4]])
		row[columns[5]] = beta[4] * float(row[columns[5]])
		row[columns[6]] = beta[5] * float(row[columns[6]])
		row[columns[7]] = beta[6] * float(row[columns[7]])
		row[columns[8]] = beta[7] * float(row[columns[8]])
		row[columns[9]] = beta[8] * float(row[columns[9]])
		row[columns[10]] = beta[9] * float(row[columns[10]])
		row[columns[11]] = beta[10] * float(row[columns[11]])
		row[columns[12]] = beta[11] * float(row[columns[12]])
		row[columns[13]] = beta[12] * float(row[columns[13]])
		row[columns[14]] = beta[13] * float(row[columns[14]])
		row[columns[15]] = beta[14] * float(row[columns[15]])
		row[columns[16]] = beta[15] * float(row[columns[16]])
		row[columns[17]] = beta[16] * float(row[columns[17]])
		row[columns[18]] = beta[17] * float(row[columns[18]])
		row[columns[19]] = beta[18] * float(row[columns[19]])
		row[columns[20]] = beta[19] * float(row[columns[20]])
		row[columns[21]] = beta[20] * float(row[columns[21]])
		row[columns[22]] = beta[21] * float(row[columns[22]])
		row[columns[23]] = beta[22] * float(row[columns[23]])
		row[columns[24]] = beta[23] * float(row[columns[24]])
		row[columns[25]] = beta[24] * float(row[columns[25]])
		row[columns[26]] = beta[25] * float(row[columns[26]])
		row[columns[27]] = beta[26] * float(row[columns[27]])
		row[columns[28]] = beta[27] * float(row[columns[28]])
		row[columns[29]] = beta[28] * float(row[columns[29]])
		lasso = 0.0
		
		for i in range(0,len(row)):
			lasso = lasso + row[columns[i]]
		
		row['LASSO-time'] = lasso
		
		data.append(row)
		
print "Removing zero'ed columns"
wanted_columns = []

for col in range(len(columns)):
	sum = 0.0
	for row in data:
		sum = sum + abs(row[columns[col]])
	if sum > 0:
		print '\tSelecting non-zero column ' + columns[col]
		wanted_columns = wanted_columns + [columns[col]]

filename = "csv/lasso-lambda-" + str(lambda_val) + ".csv"
print "Saving back to " + filename

weighted = open(filename, 'wb')
wr = csv.DictWriter(weighted, fieldnames=['LASSO-time'] + wanted_columns, delimiter=',',quotechar='"', quoting=csv.QUOTE_NONNUMERIC, extrasaction='ignore')
wr.writeheader()
for row in data:
     wr.writerow(row)
