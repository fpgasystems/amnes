import subprocess
import ast
import numpy as np
import pandas as pd

arg1 = 16
arg2 = 1024

limit = 3000000
num_iterations = 100
time_array = np.array([])

while arg2 < limit:
    for i in range(num_iterations):
        # Call script gpu_corr 
        output = subprocess.check_output(['python3', 'gpu_corr.py', str(arg1), str(arg2)])
        
        # Convert the output to dictionary
        result = float(output.decode('utf-8').strip())
        formated_result = f"{result:.4f}"
        time_array = np.append(time_array, formated_result)
    
    # Sort the array
    print(time_array)
    time_array_sorted = np.sort(time_array)
    mint  = time_array_sorted[0]
    maxt  = time_array_sorted[num_iterations-1]
    meant = 0.0 #np.average(time_array)
    medit = time_array_sorted[int(num_iterations/2)-1]

    # Print to a file
    with open('gpu_performance.txt','a') as f:
        f.write(f'{arg1}, {arg2}, {num_iterations}, {mint}, {maxt}, {medit}, {meant}\n')  

    arg2 *= 2
    time_array = np.empty(0)
