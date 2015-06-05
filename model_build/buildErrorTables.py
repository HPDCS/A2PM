#!/bin/python

import pickle
import sys
import os
import csv
import math

class Datapoint:
    def __init__(self):
        self.real = '.'
        self.predicted = '.'


# This is the reader for Lasso datapoints
def readLassoFile(fname):
    datapoints = []
    i=0;
    fd = open(fname, 'r')
    lines = fd.readlines()
    for line in lines:
        if len(line)>0 and i>0 and i<3748:
       	    tmp = line.split(',')
            if len(tmp)>0:
                dp = Datapoint() 
                dp.predicted = float(tmp[0])
                dp.real = float(tmp[1])
                datapoints.append(dp)
	i=i+1
    fd.close()
    return datapoints


# This is the reader for Weka datapoints	
def readWekaFile(fname):
	datapoints = []
	fd = open(fname, 'r')
	lines = fd.readlines()
	for line in lines:
		if len(line)>0:
			tmp = line.split()
			if len(tmp)>0:
				dp = Datapoint() 
				dp.real = float(tmp[1])
				dp.predicted = float(tmp[2])
				datapoints.append(dp)
	fd.close()
	return datapoints


def formatLassoName(string):
	value = ""
	number = float(string[13:])

	if(number == 0):
		value = "0"
	elif(number < 1):
		value = str(number)
	elif(number < 100):
		value = str(int(number))
	else:
		value = "10^" + str(int(math.log10(number)))
	
	return "Lasso ($\lambda = " + value + "$)"
	
	
def formatName(string):
	if(string == "linear"):
		return "Linear Regression"
	elif(string == "m5p"):
		return "M5P"
	elif(string == "neural"):
		return "Neural Networks"
	elif(string == "svm"):
		return "SVM"
	elif(string == "svm2"):
		return "SVM2"
	elif(string == "reptree"):
		return "REP Tree"
	else:
		return string


def MAPE():
	out_file = open("mape.gen.tex", "w")
	for f in wekafiles:
		datapoints = readWekaFile("data/" + f)
		max_error=0
		
		for dp in datapoints:
			if max_error < abs(dp.predicted-dp.real):
				max_error = abs(dp.predicted-dp.real)
		
		out_file.write(formatName(os.path.splitext(f)[0]) + "\t&\t" + str(max_error) + "\\\\\n")
	
	for f in lassofiles:
		datapoints = readLassoFile("csv/" + f)
		max_error = 0
		
		for dp in datapoints:
			if max_error < abs(dp.predicted - dp.real):
				max_error = abs(dp.predicted  -dp.real)
		
		out_file.write(formatLassoName(os.path.splitext(f)[0]) + "\t&\t" + str(max_error) + "\\\\\n")
		
	out_file.close()


def RAPE():
	out_file = open("rape.gen.tex", "w")
	for f in wekafiles:
		datapoints = readWekaFile("data/" + f)
		n=0
		sum_numerator_RAPE=0
		sum_denominator_RAPE=0
		sum_average_real_values_RAPE=0
		for dp in datapoints:
			n = n + 1
			sum_average_real_values_RAPE = sum_average_real_values_RAPE + dp.real
		average_real_values_RAPE = sum_average_real_values_RAPE / n
		for dp in datapoints:
			sum_numerator_RAPE=sum_numerator_RAPE+abs(dp.predicted - dp.real)
			sum_denominator_RAPE=sum_denominator_RAPE+abs(average_real_values_RAPE - dp.real)
		
		out_file.write(formatName(os.path.splitext(f)[0]) + "\t&\t" + str(sum_numerator_RAPE / sum_denominator_RAPE * 100) + "\\% \\\\\n")
	
	for f in lassofiles:
		datapoints = readLassoFile("csv/" + f)
		n = 0
		sum_numerator_RAPE=0
		sum_denominator_RAPE=0
		sum_average_real_values_RAPE=0
		for dp in datapoints:
			n = n + 1
			sum_average_real_values_RAPE = sum_average_real_values_RAPE+dp.real
		average_real_values_RAPE = sum_average_real_values_RAPE / n
		for dp in datapoints:
			sum_numerator_RAPE = sum_numerator_RAPE+abs(dp.predicted - dp.real)
			sum_denominator_RAPE = sum_denominator_RAPE+abs(average_real_values_RAPE - dp.real)
		
		out_file.write(formatLassoName(os.path.splitext(f)[0]) + "\t&\t" + str(sum_numerator_RAPE / sum_denominator_RAPE * 100) + "\\% \\\\\n")
		
	out_file.close()


def MAE():
	out_file = open("mae.gen.tex", "w")
	for f in wekafiles:
		datapoints = readWekaFile("data/" + f)
		sum_MAE = 0
		n = 0
		for dp in datapoints:
			n = n + 1
		for dp in datapoints:
			sum_MAE = sum_MAE + abs(dp.predicted - dp.real)
		
		out_file.write(formatName(os.path.splitext(f)[0]) + "\t&\t" + str(sum_MAE / n) + "\\\\\n")
	
	for f in lassofiles:
		datapoints = readLassoFile("csv/" + f)
		sum_MAE = 0
		n = 0
		for dp in datapoints:
			n = n + 1
		for dp in datapoints:
			sum_MAE = sum_MAE + abs(dp.predicted - dp.real)
		
		out_file.write(formatLassoName(os.path.splitext(f)[0]) + "\t&\t" + str(sum_MAE / n) + "\\\\\n")
		
	out_file.close()


def SMAE():
	out_file = open("smae.gen.tex", "w")
	for f in wekafiles:
		datapoints = readWekaFile("data/" + f)
		sum_S_MAE = 0
		n = 0
		for dp in datapoints:
			n = n + 1
		for dp in datapoints:
			if abs(dp.predicted - dp.real) > 0.1 * dp.real:
				sum_S_MAE = sum_S_MAE+abs(dp.predicted - dp.real)
		
		out_file.write(formatName(os.path.splitext(f)[0]) + "\t&\t" + str(sum_S_MAE / n) + "\\\\\n")
	
	for f in lassofiles:
		datapoints = readLassoFile("csv/" + f)
		sum_S_MAE = 0
		n = 0
		for dp in datapoints:
			n = n + 1
		for dp in datapoints:
			if abs(dp.predicted - dp.real) > 0.1 * dp.real:
				sum_S_MAE = sum_S_MAE+abs(dp.predicted - dp.real)
		
		out_file.write(formatLassoName(os.path.splitext(f)[0]) + "\t&\t" + str(sum_S_MAE / n) + "\\\\\n")
		
	out_file.close()


def SMAE_ABS():
	out_file = open("smae_abs.gen.tex", "w")
	for f in wekafiles:
		datapoints = readWekaFile("data/" + f)
		sum_S_MAE_absolute = 0
		n = 0
		for dp in datapoints:
			n = n + 1
		for dp in datapoints:
			if abs(dp.predicted - dp.real) > 300:
				sum_S_MAE_absolute=sum_S_MAE_absolute+abs(dp.predicted-dp.real)
		
		out_file.write(formatName(os.path.splitext(f)[0]) + "\t&\t" + str(sum_S_MAE_absolute / n) + "\\\\\n")
	
	for f in lassofiles:
		datapoints = readLassoFile("csv/" + f)
		sum_S_MAE_absolute = 0
		n = 0
		for dp in datapoints:
			n = n + 1
		for dp in datapoints:
			if abs(dp.predicted - dp.real) > 300:
				sum_S_MAE_absolute=sum_S_MAE_absolute+abs(dp.predicted-dp.real)
		
		out_file.write(formatLassoName(os.path.splitext(f)[0]) + "\t&\t" + str(sum_S_MAE_absolute / n) + "\\\\\n")
		
	out_file.close()



wekafiles = [ f for f in os.listdir("data") if os.path.isfile(os.path.join("data",f)) ]
lassofiles = [ f for f in os.listdir("csv") if os.path.isfile(os.path.join("csv",f)) ]

print("Computing Maximum Absolute Prediction Error")
MAPE()
print("Computing Relative Absolute Prediction Error")
RAPE()
print("Computing Mean Absolute Prediction Error")
MAE()
print("Computing Soft-Mean Absolute Prediction Error (10% threshold)")
SMAE()
print("Computing Soft-Mean Absolute Prediction Error (300 seconds threshold)")
SMAE_ABS()
