#include <iostream>
#include <string>
#include <malloc.h>
#include <time.h> 
#include <sys/time.h>  
#include <chrono>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <iomanip>
#ifdef EN_AVX
#include <x86intrin.h>
#endif
#include <boost/program_options.hpp>
#include <numeric>
#include <stdlib.h>
#include <vector>

#include "cBench.hpp"
#include "cProc.hpp"

#define DATA 8

#ifndef DATA

    typedef uint32_t data_t;
    typedef uint64_t acc_t;
#else

    #if DATA == 8

        typedef uint8_t data_t;
        typedef uint32_t acc_t;
    #endif
    
    #if DATA == 16

        typedef uint16_t data_t;
        typedef uint32_t acc_t;
    #endif
    
    #if DATA == 32

        typedef uint32_t data_t;
        typedef uint64_t acc_t;
    #endif
#endif


unsigned long get_filesize(FILE *f)
{
    long size;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    if (size < 0)
      exit(1);

    return size;
}

using namespace std;
using namespace fpga;

/* Static params */
constexpr auto const targetRegion = 0;
constexpr auto const nReps = 100;
constexpr auto const maxSize = 16 * 1024;
constexpr auto const defOper = 0; // 0: RD, 1: WR
constexpr auto const clkNs = 3.3;
constexpr auto const defSize = 128; // 2^7
constexpr auto const nBenchIncr = 8; // 2^14
constexpr auto const nBenchRuns = 100;  


/**
 * @brief Benchmark API
 * 
 */
enum class BenchRegs : uint32_t {
    CTRL_REG = 0,
    DONE_REG = 1,
    TIMER_REG = 2,
    VADDR_REG = 3,
    LEN_REG = 4,
    PID_REG = 5,
    N_REPS_REG = 6,
    N_BEATS_REG = 7
};

enum class BenchOper : uint8_t {
    START_RD = 0x1,
    START_WR = 0x2
};

union {
    float f;
    unsigned int u;
} myun;

/**
 * @brief Average it out
 * 
 */
double vctr_avg(std::vector<double> const& v) {
    return 1.0 * std::accumulate(v.begin(), v.end(), 0LL) / v.size();
}

void initData(void* block, uint32_t streams, uint32_t items, uint32_t dataSource, void* file);

int main(int argc, char *argv[])  
{
    // ---------------------------------------------------------------
    // Args 
    // ---------------------------------------------------------------
    // sudo ./main -i 100 
    // sudo ./main -i 100000 -s 1 -f ../test_int_16_100000.csv
    // Read arguments
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()("items,i", boost::program_options::value<uint32_t>(), "items")
                                    ("repetitions,r", boost::program_options::value<uint32_t>(), "repetitions")
                                    ("streams,s", boost::program_options::value<uint32_t>(), "Number of streams")
                                    ("filename,f", boost::program_options::value<std::string>(), "filename")
                                    ("datatype,d", boost::program_options::value<uint32_t>(),"datatype")
                                    ("datasource,a",boost::program_options::value<uint32_t>(),"datasource");
       
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    uint32_t items = 0;
    if (commandLineArgs.count("items") > 0) {
        items = commandLineArgs["items"].as<uint32_t>();
    } else {
      std::cerr << "Number of items required.\n";
      return -1;
    }
    
    uint32_t repetitions = 1;
    if (commandLineArgs.count("repetitions") > 0) {
        repetitions = commandLineArgs["repetitions"].as<uint32_t>();
    }

    uint32_t streams = 16;
    if (commandLineArgs.count("streams") > 0) {
        streams = commandLineArgs["streams"].as<uint32_t>();
    }

    std::string filename;
    FILE *file = NULL;
    if (commandLineArgs.count("filename") > 0){
        filename = commandLineArgs["filename"].as<std::string>();
        std::cout<<"Filename: "<<filename<<std::endl;

        file = fopen((const char*)filename.c_str(), "r");
        if(!file){
            std::cerr<<"Could not open the file.\n";
            return -1;
        }
    }

    uint32_t dataType = 0;
    if (commandLineArgs.count("datatype") > 0){
        dataType = commandLineArgs["datatype"].as<int32_t>();
    }
    
    uint32_t dataSource = 0;
    if (commandLineArgs.count("datasource") > 0){
        dataSource = commandLineArgs["datasource"].as<uint32_t>();

        if(dataSource!=0 and filename.empty()){
            std::cerr<<"File has to be mentioned for non locally generated sources. \n";
            return -1;
        }
    }
     
    uint32_t n_data_pages, n_result_pages;

    int data_size   = items * streams * sizeof(data_t);
    int result_size = streams*(streams-1)/2 * 16 * sizeof(float);

    n_data_pages = data_size/hugePageSize   + ((data_size%hugePageSize > 0)? 1 : 0);
    n_result_pages = result_size/hugePageSize + ((result_size%hugePageSize > 0)? 1 : 0);
    
    std::cout<< "-- PARAMS -------------------------------------" << std::endl;

    std::cout<< "Items/Stream: " << items << std::endl;
    std::cout<< "Streams     : " << streams << std::endl;

    if(!filename.empty())
        std::cout<<"Source: locally generated \n";
    else
        std::cout<<"File name    : "<<filename<<std::endl;

    std::cout<< "-----------------------------------------------"<<std::endl;
    std::cout<< "vFPGA ID: " << targetRegion << std::endl;
    std::cout<< "Number of allocated data pages: " << n_data_pages<<std::endl;
    std::cout<< "Number of allocated result pages: " << n_result_pages<<std::endl;

    // ---------------------------------------------------------------
    // Init 
    // ---------------------------------------------------------------
    // Acquire a region
    cProc cproc(targetRegion, getpid());

    // Memory allocation
    void* dataMem = cproc.getMem({CoyoteAlloc::HOST_2M, n_data_pages});
    void* resultMem = cproc.getMem({CoyoteAlloc::HOST_2M, n_result_pages});

    std::cout<< "Data memory mapped at: " << dataMem <<std::endl;
    std::cout<< "Result memory mapped at: " << resultMem <<std::endl;
    
    // Initialize data 
    initData(dataMem, streams, items, dataSource, file);
    
    // Set the parameters
    cproc.setCSR(items, 2);

    std::cout<<"Set parameters"<<std::endl;
    std::cout<<"Get register val:"<<cproc.getCSR(2)<<std::endl;

    std::vector<double> duration;  // ns

    // Start measuring
    for (uint32_t reps=0; reps<repetitions; reps++) {

        // Reset result data
        memset(resultMem, 0, result_size);
        auto begin_time = std::chrono::high_resolution_clock::now();
    
        // Stream data into the FPGA, non-blocking, initiate transfer in both directions (results writen back)
        //cproc.invoke({CoyoteOper::READ, (void*)dataMem, (uint32_t)data_size, true, false});
        
        // Write results from the FPGA as they come, blocking, returns when all results have been writen to the host
        //cproc.invoke({CoyoteOper::WRITE, (void*)resultMem, (uint32_t)result_size});
        cproc.invoke({CoyoteOper::TRANSFER, (void*)dataMem, (void*)resultMem, (uint32_t)data_size, (uint32_t)result_size, true, false});
        bool k = false, r = false;
        while(!r){
            if(cproc.checkCompleted(CoyoteOper::TRANSFER) != 1 ) k = true;
    
            if(*((uint32_t *)(resultMem)+16*(streams*(streams-1)/2-1))!=0){
                //std::cout<<"Test value:"<<*((uint32_t *)resultMem)<<std::endl;
                r = true;
            }
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        
        double const d0 = (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - begin_time).count());

        duration.push_back(d0);

        //std::cout << dec << "\nTime [us]: " << d0 << " us" << std::endl;
        //double throughput = ((double) data_size/d0)/1000.0;
        //std::cout << dec << "Throughput [GB/s]: " << throughput <<  std::endl;

        sleep(1);
    }

//    std::cout<<"Results:"<<std::endl;
//    float* res = (float *) resultMem;
//    for(uint32_t i=0; i<4; i++){
//        for(uint32_t j=0;j<1;j++){
//        myun.f = *(res+16*i);
//        std::cout<<"Correlation coefficient float:"<<myun.f<<" binary:";
//        for(unsigned int ra=0x80000000; ra; ra>>=1){
//            if(ra==0x40000000) std::cout<<" ";
//            if(ra==0x00400000) std::cout<<" ";
//            if(ra&myun.u) std::cout<<"1"; else std::cout<<"0";
//        }
//        std::cout<<std::endl;
//        }        
//    }
//    std::cout<<std::endl;


    std::sort(duration.begin(),duration.end());

    uint32_t divider = 1000;

    double min_lat = duration[0]/divider;
    double max_lat = duration[repetitions-1]/divider;
    double min_thr = data_size/duration[repetitions-1];
    double max_thr = data_size/duration[0];

    double p25_lat = 0.0;
    double p50_lat = 0.0;
    double p75_lat = 0.0;

    double p25_thr = 0.0;
    double p50_thr = 0.0;
    double p75_thr = 0.0;

    if(repetitions>=4){

        p25_lat = duration[(repetitions/4)-1]/divider;
        p50_lat = duration[(repetitions/2)-1]/divider;
        p75_lat = duration[(repetitions*3)/4-1]/divider;

        p75_thr = data_size/duration[(repetitions/4)-1];
        p50_thr = data_size/duration[(repetitions/2)-1];
        p25_thr = data_size/duration[(repetitions*3)/4-1];
    }

    double p1_lat  = 0.0;
    double p5_lat  = 0.0;
    double p95_lat = 0.0;
    double p99_lat = 0.0;

    double p1_thr  = 0.0;
    double p5_thr  = 0.0;
    double p95_thr = 0.0;
    double p99_thr = 0.0;

    if (repetitions >= 100) {

        p1_lat  = duration[((repetitions)/100)-1]/divider;
        p5_lat  = duration[((repetitions*5)/100)-1]/divider;
        p95_lat = duration[((repetitions*95)/100)-1]/divider;
        p99_lat = duration[((repetitions*99)/100)-1]/divider;

        p99_thr = data_size/duration[((repetitions)/100)-1];
        p95_thr = data_size/duration[((repetitions*5)/100)-1];
        p5_thr  = data_size/duration[((repetitions*95)/100)-1];
        p1_thr  = data_size/duration[((repetitions*99)/100)-1];
    }    

    FILE *resultFile;
    std::string name = "amnes_coprocessor_" + std::to_string(streams) + ".txt";

    resultFile = fopen(name.c_str(), "a");

    //input files size
    unsigned int file_size_result = get_filesize(resultFile);
    if (file_size_result == 0)
        fprintf(resultFile, "streams streamitems repeat lmin lmax lp1 lp5 lp25 lp50 lp75 lp95 lp99 tmin tmax tp1 tp5 tp25 tp50 tp75 tp95 tp99 \n");
 
    fseek(resultFile, file_size_result, SEEK_END); 
    fprintf(resultFile,"%u %u %u %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f \n",
            streams, items, repetitions, 
            min_lat, max_lat, p1_lat, p5_lat, p25_lat, p50_lat, p75_lat, p95_lat, p99_lat,
            min_thr, max_thr, p1_thr, p5_thr, p25_thr, p50_thr, p75_thr, p95_thr, p99_thr);
    fclose(resultFile);

    duration.clear();

    return EXIT_SUCCESS;
}

void initData(void* block, uint32_t streams, uint32_t items, uint32_t dataSource, void* file) {
    
    FILE* localFile = (FILE*) file;
    data_t* dataBlock = (data_t*) block;

    if (dataSource==0) {
        data_t v = 0;
        for(uint32_t i=0; i<items; i++){
            for(uint32_t j=0; j<streams; j++)
                *(dataBlock+streams*i+j) = v;

            v++;
        }
    }
    else {
        char content[1024];
        int i = 0;
        int j = 0;
        
        while(fgets(content, 1024, localFile)){

            char *v = strtok(content, ",");
            j = 0;

            while (v){

                if(i>0 and j>0){
                   *(dataBlock + streams*(i-1) + (j-1)) = (data_t)atoi(v); 
                   // std::cout<<"i,j,v:"<<i-1<<","<<j-1<<","<<values[i-1][j-1]<<std::endl;             
                }

                v =strtok(NULL, ",");
                j++;
            }
            i++;          
        }
        fclose(localFile);

    }
}
