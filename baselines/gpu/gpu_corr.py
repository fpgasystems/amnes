import torch 
import numpy as np

import time
import sys
import os
import glob

#script call: python3 gpu-corr.py <streams> <elements>
streams  = 16;
elements = 1024;

if len(sys.argv) > 1:
        streams = int(sys.argv[1])

        if len(sys.argv) > 2:
            elements = int(sys.argv[2])

#print("Streams: ",streams)
#print("Items  :",elements)

# Initialize matrix
matrix = np.zeros((streams, elements))

# Loop through each column
for col_idx in range(streams):
    matrix[col_idx,:] = np.arange(1,elements+1).astype(float)/10000 # astype(int)

xtensor = torch.tensor(matrix)
cuda0 = torch.device('cuda:0') 
#xtensor = xtensor.to(cuda0)

# Perform some operation on the tensor
torch.cuda.synchronize()
start_time = time.perf_counter_ns()

corr_coeffs = torch.corrcoef(xtensor)

torch.cuda.synchronize()
stop_time = time.perf_counter_ns()

# Execution time
execution_time = (stop_time - start_time)/1000 #[us]
print(execution_time)
