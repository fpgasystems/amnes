# Correlation computation [AMNES]

Design space exploration of correlation computation implementation on FPGA. 

### baselines
#### cpp 
```
$ cd baselines/cpp
$ mkdir build
$ cmake ../src/ -DDEBUG=1   // for debugging
$ cmake ../src/ -DDEBUG=0 -DMACHINE_NAME=r740-01 -DTESTOPT=1 // throughput benchmark
$ make
$ make amnes_throughput
```
#### outer_lib
```
$ cd baselines/outer_lib
$ git clone https://gitlab.com/libeigen/eigen.git eigen340
$ cd eigen340 && git checkout 3.4.0
$ cd ..
$ g++ -march=native -O3 -ffast-math -mstackrealign -I eigen340 src/ss_eigenlib.cpp -o ss_eigenlib
$ ./ss_eigenlib 1048576 16 32 1 100 && ./ss_eigenlib 2097152 16 32 1 100 && ./ss_eigenlib 4194304 16 32 1 100 && ./ss_eigenlib 8388608 16 32 1 100
$ cat ss_tput_cllct_16_32.txt

$ g++ -march=native -O3 -pthread -ffast-math -mfma -mstackrealign -I eigen340 src/ss_eigenlib_th.cpp -o ss_eigenlib_th
$ ./ss_throughput
```

#### gpu - PyTorch v2.0.0
```
$ cd baselines/gpu

$ mkdir test-gpu
$ python3 -m venv test-gpu
$ source test-gpu/bin/activate  
$ python3 mcallsgpu.py
$ cat gpu_performance.txt

```

#### DBs
##### Postgres - v15.2
[Download](https://www.postgresql.org/ftp/source/v15.2/)

$ /configure --prefix="<install-location>"
$ make
$ make install
$ export PGHOST = localhost
$ bin/pg_ctl -D <absolute-data-directory> initdb
$ bin/pg_ctl -D <absolute-data-directory> (start | stop)
$ bin/psql postgre
$ \timing on
$ \i dbs/test-corr-postgres.sql

##### Snowflake
$ cat dbs/test-corr-snowflake.txt

##### MySQL - v8.0.33
$ cat dbs/test-corr-mysql.txt