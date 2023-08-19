import pandas as pd
import numpy as np
import time
import resource
import sys
import os
import glob
import argparse
import random
import math
import array
from matplotlib import pyplot as plt

# script call: python3 create_test_dataset.py -t float

# define functions 
a = random.randrange(-10,10)
b = random.randrange(-15,15)
c = random.randrange(-10,10)
d = random.randrange(-10,10)

# linear
def col0(x):
#	return (0)
	return (x/pow(10,1)+2)

def col1(x):
#	return (1)
	return (-5*x/pow(10,2)+4)

def col2(x):
#	return (2)
	return (-2*pow(x,2)/pow(10,3)+3)

def col3(x):
#	return (3)
	return (-5*x/pow(10,2)+20)

# cubic
def col4(x):
	a = random.randrange(-2,2)
	b = random.randrange(-15,15)
	c = random.randrange(-10,10)
#	return (4)
	return (pow(x,3)/pow(10,4)-a*pow(x,2)/pow(10,3)+b*x/pow(10,1)+c)

# quartic
def col5(x):
	d = random.randrange(-2,2)
#	return (5)
	return (pow(x,4)/pow(10,6)-4*pow(x,3)/pow(10,4)+4*pow(x,2)/pow(10,3)-x/pow(10,2)+d)

# quintic
def col6(x):
	d = random.randrange(-4,5)
	return pow(x,5)/pow(10,5)-5*pow(x,4)/pow(10,4)-5*pow(x,3)/pow(10,3)-5*pow(x,2)/pow(10,2)-6*x/pow(10,1)+d
	return (d)

# square root
def col7(x):
	if(x<0):
		x = (-1)*x
#	return (7)
	return (math.sqrt(x)/pow(10,2))

def col8(x):
	d = random.randrange(-4,5)
	return (d+a)

def col9(x):
#	return (9)
	return (-5*x/pow(10,2)+4)

def col10(x):
#	return (10)
	return (-b*pow(x,2)/pow(10,3)+a*pow(x,1)/pow(10,2)+c)

def col11(x):
#	return(11)
	return (2*pow(x,3)/pow(10,4)-3*x/pow(10,2)+20)

def col12(x):
	c = random.randrange(-10,10)
#	return(12)
	return (math.log10(x)+c*x)

# quartic
def col13(x):
	d = random.randrange(-2,2)
#	return(13)
	return (pow(x,4)/pow(10,6)+4*pow(x,3)/pow(10,4)+d)

# quintic
def col14(x):
	d = random.randrange(-4,5)
#	return(14)
	return (-6*x/pow(10,3)+d)

# square root
def col15(x):
	if (x<0):
		x = (-1)*x
	return (math.sqrt(x)/pow(10,5))

parser = argparse.ArgumentParser()
parser.add_argument('-c', action='store')  # column size
parser.add_argument('-r', action='store')  # row size
parser.add_argument('-t', action='store')  # data type
parser.add_argument('-ll', action='store')  # low limit
parser.add_argument('-hl', action='store')  # high limit

args = parser.parse_args()

columns    = 2
rows       = 10
low_limit  = 1
high_limit = 10 
step       = 0

if args.c is not None:
	columns = int(args.c)
	if columns < 2:
		columns = 2

if args.r is not None:
	rows = int(args.r)
	if rows < 10:
		rows = 10

if args.t is None:
	data_type = 'int'
else:
	data_type = args.t

if args.ll is not None:
	low_limit = float(args.ll)

if args.hl is not None:
	high_limit = float(args.hl)

filename = 'test_'+str(data_type)+'_'+str(columns)+'_'+str(rows)+'.csv'
current_directory = os.getcwd()

step = (high_limit-low_limit)/rows

data_matrix = [[0 for x in range(columns)] for y in range(rows)]

column_names = []
x_values = [0 for x in range(rows)]
y_values = [0 for x in range(rows)]

for i in range (columns):
	column_item = 'a'+str(i)
	column_names.append(column_item)

for i in range(rows):
	x_values[i] = float(low_limit)

	for j in range(columns):
		function_name = 'col'+str(j)
		data_matrix[i][j] = eval(data_type)(eval(function_name)(low_limit))
		if (i==0):
			y_values[i] = data_matrix[i][j]
		low_limit = low_limit + step

df = pd.DataFrame(data=data_matrix, columns=vars()["column_names"])

#for i in range(columns):
#	plt.plot(x_values,df[column_names[i]])

#plt.show()

csv_directory = os.path.join(current_directory, 'test')
df.to_csv(os.path.join(csv_directory, filename))