import pandas as pd
import numpy as np
import time
import resource
import sys
import os
import glob

# script call: python3 correlation.py pandas/numpy int/float x(repetitions)
# default:
pearson_np  = 0;
repetitions = 100
data_type   = 'int'

if len(sys.argv) > 1:
	if sys.argv[1]=='pandas':
		pearson_np = 0;
		print("Compute correlation with pandas")
	else:
		if sys.argv[1]=='numpy':
			pearson_np = 1;
			print("Compute correlation with numpy")
		else:
			print("pandas/numpy as first argument to the call")
    
	if len(sys.argv) > 2:
		data_type = sys.argv[2]

	if len(sys.argv) > 3:
		repetitions = int(sys.argv[3])
	
else:
	print("Compute correlation with pandas")

print("Repetitions:",repetitions)
# performance-time in [ms]; performance-memory in [MB]

df_perf = pd.DataFrame(columns=['dataset','no_elements','no_attributes','repetitions','time','memory'])
elements_array = [100,300,400,500,1000,3000,4000,5000,10000,10000,30000,40000,50000,100000,200000]

dataset_names = ['test'] #'beijing','madrid',
attributes_beijing = ['PM2.5','PM10','SO2','NO2','CO','O3','TEMP','PRES']
attributes_madrid  = ['CO','NO_2','O_3','PM10','PM25','SO_2']
attributes_test    = ['a0','a1','a2','a3','a4','a5','a6','a7','a8','a9','a10','a11','a12','a13','a14','a15']

current_directory = os.getcwd()
final_directory = os.path.join(current_directory, r'results')
if not os.path.exists(final_directory):
	os.makedirs(final_directory)

for name in dataset_names:
	#directory = os.path.join(os.sep,'mnt','scratch','mchiosa','Datasets','Correlation','air_quality_'+name) #'air_quality_beijing'
	directory = os.path.join(current_directory,'test')
	print ("Path:", directory)
	var = "attributes_%s"%name
	print (var,":",vars()[var])
	
	for root, dirs, files in os.walk(directory):
		for file in files:
			if file.endswith(".csv"):
				print (file)
				dataset = pd.read_csv(os.path.join(directory, file))
				total_elements = dataset.shape[0]   # it includes also the header
				total_attributes = dataset.shape[1] # it includes also the index column

				for elements in elements_array:
					if elements <= total_elements:
						total_time = 0.0
						total_memory = 0.0
						
						dataset = pd.read_csv(os.path.join(directory, file), nrows=elements, usecols=vars()[var])
						# print(dataset.dtypes)
						
						local_elements = dataset.shape[0]
						local_attributes = dataset.shape[1]

						if pearson_np == 1:
							data_column = [[0 for x in range(local_attributes)] for y in range(local_elements)]
					
							for i in range(local_attributes):
								miss_value = eval(data_type)(0.0)

							data_column = dataset[vars()[var]].to_numpy(na_value=miss_value)
							# print(type(data_column[0][0]))

						for i in range(repetitions):
							#pairwise correlation
							if pearson_np == 1:
								time_start     = time.perf_counter()
								corr_matrix_np = np.corrcoef(data_column, rowvar=False) 
								time_elapsed   = (time.perf_counter() - time_start)*1000                     # [ms]
								mem_used       =resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1024.0/1024.0 # [MB]

							else:
								time_start   = time.perf_counter()
								corr_matrix  = dataset.corr(method='pearson') 
								time_elapsed = (time.perf_counter() - time_start)*1000               # [ms]
								mem_used     =resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1024.0/1024.0 # [MB]
		
							total_time += time_elapsed
							total_memory += mem_used
		
						execution_time = total_time/repetitions
						memory = total_memory/repetitions 
		
						df_perf.loc[-1] = [file, elements, len(vars()[var]),repetitions, execution_time, memory]
						df_perf.index = df_perf.index+1
					
						if (pearson_np==0):
							filename_pcc = "pcc_%s"%(str(elements)+'_'+file)
							filename_perf= 'result_performance.dat'
						else:
							filename_pcc = "pcc_%s"%(str(elements)+'_'+'np_'+file)
							filename_perf= 'result_performance_np.dat'
					
						df_perf.to_csv(os.path.join(final_directory, filename_perf))
						if (pearson_np==0):
							corr_matrix.to_csv(os.path.join(final_directory, filename_pcc))
						else:
							df = pd.DataFrame(corr_matrix_np, index=None, columns=None)
							df.to_csv(os.path.join(final_directory, filename_pcc))
