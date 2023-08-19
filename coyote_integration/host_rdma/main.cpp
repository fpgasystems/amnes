// Command examples
// ./main -s 16 -m 64 -p 4096 -w 1 -r 1
// ./main -t 10.1.212.175 -s 16 -m 64 -p 4096 -w 1 -r 1

#include "cDefs.hpp"

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
#include <random>
#include <cstring>
#include <signal.h> 
#include <atomic>
#include <vector>
#include <thread>

#include <boost/program_options.hpp>

#include "cBench.hpp"
#include "ibvQpMap.hpp"

#define CORR_SW   0
#define CORR_FPGA 1
#define CORR_NONE 0
#define DEBUG     0

#if(CORR_SW == 1)
    #include "correlation.hpp"
#endif

using namespace std;
using namespace std::chrono;
using namespace fpga;

#if(CORR_FPGA ==1)

    enum class AmnesRegs : uint32_t {
        ITEMS_REG = 2,
        VADDR_REG = 4,
        LEN_REG = 5,
        CHLINE_REG = 6,
    };
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

/* Signal handler */
std::atomic<bool> stalled(false); 
void gotInt(int) {
    stalled.store(true);
}

void fillData(void* block, uint32_t streamItems, uint32_t streams){

    uint32_t* uPtr = (uint32_t *)block;
    uint32_t     v = 1;

    for(unsigned i=0; i<streamItems; i++) {
        for(unsigned j=0; j<streams; j++)
            *(uPtr+i*streams + j) = v;

        v++;
    }
}

/* Params */
constexpr auto const targetRegion = 0;
constexpr auto const qpId = 0;
constexpr auto const port = 18488;

/* Bench */
constexpr auto const defNBenchRuns = 1; 
constexpr auto const defNReps = 10;
constexpr auto const defMinSize = 4096;
constexpr auto const defMaxSize = 4096;
constexpr auto const defOper = 0;

int main(int argc, char *argv[])  
{
    // ---------------------------------------------------------------
    // Initialization 
    // ---------------------------------------------------------------

    // Sig handler
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = gotInt;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

    // Read arguments
    boost::program_options::options_description programDescription("Options:");
    programDescription.add_options()
        ("tcpaddr,t", boost::program_options::value<string>(), "TCP conn IP")
        ("benchruns,b", boost::program_options::value<uint32_t>(), "Number of bench runs")
        ("reps,r", boost::program_options::value<uint32_t>(), "Number of n_reps within a run")
        ("streams,s", boost::program_options::value<uint32_t>(), "Number of streams")
        ("streamitems,m", boost::program_options::value<uint32_t>(), "Items per stream")
        ("transfer,p", boost::program_options::value<uint32_t>(), "Transfer size")
        ("threads,h", boost::program_options::value<uint32_t>(), "Number of threads")
        ("chunk,c", boost::program_options::value<uint32_t>(), "Processing chunk")
        ("oper,w", boost::program_options::value<bool>(), "Read or Write");
    
    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);

    // Stat
    string tcp_mstr_ip;
    uint32_t n_bench_runs = defNBenchRuns;
    uint32_t n_reps = defNReps;

    uint32_t streams      = 16;
    uint32_t streamItems  = 64;
    uint32_t transferSize = defMinSize;
    bool oper = defOper;
    bool mstr = true;

    char const* env_var_ip = std::getenv("FPGA_0_IP_ADDRESS");
    if(env_var_ip == nullptr) 
        throw std::runtime_error("IBV IP address not provided");
    string ibv_ip(env_var_ip);

    if(commandLineArgs.count("tcpaddr") > 0) {
        tcp_mstr_ip = commandLineArgs["tcpaddr"].as<string>();
        mstr = false;
    }
    if(commandLineArgs.count("benchruns") > 0) n_bench_runs = commandLineArgs["benchruns"].as<uint32_t>();
    if(commandLineArgs.count("reps") > 0) n_reps = commandLineArgs["reps"].as<uint32_t>();
    if(commandLineArgs.count("streams") > 0) streams = commandLineArgs["streams"].as<uint32_t>();
    if(commandLineArgs.count("streamitems") > 0) streamItems = commandLineArgs["streamitems"].as<uint32_t>();
    if(commandLineArgs.count("transfer") > 0) transferSize = commandLineArgs["transfer"].as<uint32_t>();
    if(commandLineArgs.count("oper") > 0) oper = commandLineArgs["oper"].as<bool>();

    uint32_t totalTransferSize = streams * streamItems * sizeof(uint32_t);
    uint32_t n_transfers       = totalTransferSize / transferSize;
    uint32_t n_pages           = (totalTransferSize + (streams*(streams-1)<<1) + hugePageSize - 1) / hugePageSize; // To do change allocatation according to the CORR type

    PR_HEADER("PARAMS");
    if(!mstr) { std::cout << "TCP master IP address: " << tcp_mstr_ip << std::endl; }
    std::cout << "IBV IP address: " << ibv_ip << std::endl;
    std::cout << "Number of allocated pages: " << n_pages << std::endl;
    std::cout << (oper ? "Write operation" : "Read operation") << std::endl;
    std::cout << "Transfer size [B]: " << transferSize << std::endl;
    std::cout << "Total size    [B]: " << totalTransferSize << std::endl;
    std::cout << "Number of transfers: " << n_transfers << std::endl;
    std::cout << "Number of reps     : " << n_reps << std::endl;

#if(CORR_SW == 1)

    uint32_t numThreads = 1;
    uint32_t chunkSizeB = 1024;

    Eigen::initParallel();

    std::cout<<"Eigen Lib: "<<EIGEN_MAJOR_VERSION<<"."<<EIGEN_MINOR_VERSION<<std::endl;
    uint32_t rows = streams+1;
    
    MatrixXacc M3(rows,rows); M3.setZero();
    MatrixXacc M3_collect(rows,rows); M3_collect.setZero();

    float pccCoeff[streams*(streams-1)>>1];

    if(commandLineArgs.count("threads") > 0) numThreads = commandLineArgs["threads"].as<uint32_t>();
    if(commandLineArgs.count("chunk") > 0) chunkSizeB = commandLineArgs["chunk"].as<uint32_t>();

    std::vector<MatrixXacc> vector_of_M3;

    for(uint32_t i=0; i<numThreads; i++) {

        vector_of_M3.push_back(M3);
    }

    corr_chunk_args_t* argvs = (corr_chunk_args_t*)malloc(numThreads*sizeof(corr_chunk_args_t));
   
    for (uint32_t i=0; i<numThreads; i++){
        argvs[i].corr_thread = (corr_thread_args_t*)malloc(sizeof(corr_thread_args_t));
    }
   
    pthread_t* threads = (pthread_t*)malloc(numThreads*sizeof(pthread_t));
#endif

    // Create  queue pairs
    ibvQpMap ictx;
    ictx.addQpair(qpId, targetRegion, ibv_ip, n_pages);
    mstr ? ictx.exchangeQpMaster(port) : ictx.exchangeQpSlave(tcp_mstr_ip.c_str(), port);
    ibvQpConn *iqp = ictx.getQpairConn(qpId);

    // Init app layer --------------------------------------------------------------------------------
    struct ibvSge sg;
    struct ibvSendWr wr;

    // Allocate & Populate Input Memory & Allocted Output Memory

    memset(&sg, 0, sizeof(sg));
    sg.type.rdma.local_offs = 0;
    sg.type.rdma.remote_offs = 0;
    sg.type.rdma.len = transferSize;

    memset(&wr, 0, sizeof(wr));
    wr.sg_list = &sg;
    wr.num_sge = 1;
    wr.opcode = oper ? IBV_WR_RDMA_WRITE : IBV_WR_RDMA_READ;
    
    uint64_t *hMem = (uint64_t*)iqp->getQpairStruct()->local.vaddr;
    if(mstr) fillData((void*) hMem, streamItems, streams);
    if(!mstr) memset(hMem, 0, 128);
    
// Handles
#if(CORR_FPGA == 1)

    uint32_t resultSize = 512;//8;//streams*(streams-1)<<1; // (streams)(streams-1)/2*4
    uint64_t *hRst = hMem + totalTransferSize/sizeof(uint64_t);
    volatile uint64_t *lastRst = hRst + 476/sizeof(uint64_t);
    
    memset(hRst, 0, resultSize);

    cProcess *currentcProc = iqp->getCProc();

    currentcProc->setCSR(static_cast<uint64_t>(streamItems), static_cast<uint32_t>(AmnesRegs::ITEMS_REG));
    currentcProc->setCSR(reinterpret_cast<uint64_t>(hRst), static_cast<uint32_t>(AmnesRegs::VADDR_REG));
    currentcProc->setCSR(resultSize, static_cast<uint32_t>(AmnesRegs::LEN_REG));
    currentcProc->setCSR(8, static_cast<uint32_t>(AmnesRegs::CHLINE_REG)); //currentcProc->getCpid()

    std::cout << "Set Amnes Parameters:"<< std::endl;
    std::cout << "Get ITEMS register value        :" << currentcProc->getCSR(static_cast<uint32_t>(AmnesRegs::ITEMS_REG))<< std::endl;
    std::cout << "Get vaddress register value     :" << std::hex<< currentcProc->getCSR(static_cast<uint32_t>(AmnesRegs::VADDR_REG))<< std::endl;
    std::cout << "Get Result Length register value:" << std::dec<< currentcProc->getCSR(static_cast<uint32_t>(AmnesRegs::LEN_REG))<< std::endl;
    std::cout << "Get Cacheline register value    :" << currentcProc->getCSR(static_cast<uint32_t>(AmnesRegs::CHLINE_REG))<< std::endl;

#endif  
    //return EXIT_SUCCESS;
    iqp->ibvSync(mstr);
    
    PR_HEADER("Correlation BENCHMARK");

    // Setup
    iqp->ibvClear();
    iqp->ibvSync(mstr);

    std::vector<float> duration_all;     //[ns]
    std::vector<float> duration_collect; //[ns]   // use also for the FPGA
    std::vector<float> duration_merge;   //[ns]
    std::vector<float> duration_corr;    //[ns]

    // Measurements ----------------------------------------------------------------------------------
    if(mstr) {
        // Inititator 
        
        // ---------------------------------------------------------------
        // Runs 
        // ---------------------------------------------------------------
        for(uint32_t r=0; r<n_reps; r++){
#if(DEBUG) //--------------------------------------------------------------
            uint32_t* uPtr = (uint32_t*) hMem;
            std::cout<<"Pointers:\n"<< hMem<<std::endl<<uPtr<<std::endl;
            for(uint32_t i=0; i<streamItems; i++) {
                for(uint32_t j=0; j<streams; j++)
                    std::cout<<*(uPtr+i*streams+j)<<" ";
                std::cout<<std::endl;
            }
#endif //-------------------------------------------------------------------
       
            // Initiate
            auto const t0 = std::chrono::high_resolution_clock::now();

            for(int i = 0; i < n_transfers; i++) {
                iqp->ibvPostSend(&wr);
                sg.type.rdma.local_offs += transferSize;
                sg.type.rdma.remote_offs += transferSize;
            }
    
            //// Wait for completion
            //while(iqp->ibvDone() < n_transfers) { 
            //    if( stalled.load() ) throw std::runtime_error("Stalled, SIGINT caught"); 
            //}

            auto const t1 = std::chrono::high_resolution_clock::now();
            //std::cout << std::setw(5) << sg.type.rdma.len << " [bytes], thoughput: " 
            //    << std::fixed << std::setprecision(2) << std::setw(8) << ((1 + oper) * ((1000 * sg.type.rdma.len))) / ((bench.getAvg()) / n_reps) << " [MB/s], latency: "; 
                
            // Reset
            iqp->ibvClear();
            iqp->ibvSync(mstr);

            float const d0 = (float)(std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count());
        
            duration_all.push_back(d0);
            // Reset 
            sg.type.rdma.local_offs  = 0;
            sg.type.rdma.remote_offs = 0;
        }     
    } else {
        // Server
        if(oper) {  

            for(uint32_t r=0; r<n_reps; r++){

#if(CORR_SW == 1) //-------------------------------------------------------------------------------
                // Start the threads 
                for (uint32_t i = 0; i < numThreads; i++) {

                    argvs[i].startAddr = (uint32_t*)(hMem + i*chunkSizeB/sizeof(uint64_t));
                    argvs[i].memSize   = totalTransferSize;
                    argvs[i].threads   = numThreads;
                    argvs[i].chunk     = chunkSizeB;

                    argvs[i].corr_thread->pIntermResult = (uint32_t *)&vector_of_M3[i];
                    argvs[i].corr_thread->streamNumber  = streams;

                    //std::cout<<i<<" TEST Start Address:"<<argvs[i].addr <<std::endl;
                    pthread_create(&threads[i], NULL, corr_chunk, (void*)&argvs[i]);
                }
#endif //-----------------------------------------------------------------------------------------
                volatile uint64_t* ptrMem = hMem;
                while(*(ptrMem)==0);
                
                auto const t0 = std::chrono::high_resolution_clock::now();

                // Wait for incoming transactions
                while(iqp->ibvDone() < n_transfers) { 
                    if( stalled.load() ) throw std::runtime_error("Stalled, SIGINT caught");  
                }
    
                //// Send back
                //for(int i = 0; i < n_transfers; i++) {
                //    iqp->ibvPostSend(&wr);
                //} 
#if(CORR_SW == 1) //-------------------------------------------------------------------------------

                // Wait for the threads to finish
                for (uint32_t i = 0; i < numThreads; i++) {
                    pthread_join(threads[i], NULL);
                } 
                auto const t1 = std::chrono::system_clock::now();

                for (uint32_t i = 0; i < numThreads; i++) {
                    M3_collect += vector_of_M3[i];
                }
                    
                auto const t2 = std::chrono::high_resolution_clock::now();
                get_pcc_coefficients(streams, (uint32_t*) &M3_collect, pccCoeff);
#endif //-----------------------------------------------------------------------------------------
#if(CORR_FPGA == 1) //----------------------------------------------------------------------------

                auto const t3 = std::chrono::high_resolution_clock::now();            
                while(*(lastRst)==0);
                
#if(DEBUG == 1) //---------------------------------------------------------------------------------
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::cout<<"Values: "<<std::hex<<(*hRst)<<std::endl;
                std::cout<<"Values: "<<std::hex<<(void*)lastRst<<std::endl;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                uint32_t *ptrRST = (uint32_t *) hRst;
                for(uint32_t i=0; i<125; i++)
                    std::cout<<(float)(*(ptrRST+i))<<" "<<std::hex<<(ptrRST+i)<<std::endl;
#endif ////---------------------------------------------------------------------------------------
#endif //----------------------------------------------------------------------------------------- 

                auto const t4 = std::chrono::high_resolution_clock::now();
#if(DEBUG) //-------------------------------------------------------------------------------------

                uint32_t* uPtr = (uint32_t*) hMem;
                std::cout<<"Pointers:\n"<< hMem<<std::endl<<uPtr<<std::endl;
                for(uint32_t i=0; i<streamItems; i++) {
                    for(uint32_t j=0; j<16; j++)
                        std::cout<<*(uPtr+i*16+j)<<" ";
                    std::cout<<std::endl;
                } 
#endif //-------------------------------------------------------------------------------------------

                // Reset
                iqp->ibvClear();
                //std::cout << "\e[1mSyncing ...\e[0m" << std::endl;
                iqp->ibvSync(mstr);

                *(ptrMem) = 0;
                // Clear the Matrices

#if(CORR_SW == 1) //-------------------------------------------------------------------------------
    #if(DEBUG) //----------------------------------------------------------------------------------

                std::cout<<M3_collect<<std::endl;
                for(uint32_t i=0; i<(streams*(streams-1)>>2); i++)
                    std::cout<<pccCoeff[i]<< " ";
                std::cout<<std::endl;
    #endif //--------------------------------------------------------------------------------------

                for(uint32_t i=0; i < numThreads; i++)
                    vector_of_M3[i].setZero();

                M3_collect.setZero();
#endif //-------------------------------------------------------------------------------------------

                float const d0 = (float)(std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t0).count());
                duration_all.push_back(d0);
#if(CORR_SW == 1) //-------------------------------------------------------------------------------

                float const d1 = (float)(std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count());
                float const d2 = (float)(std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count());
                float const d3 = (float)(std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t2).count());
                    
                duration_collect.push_back(d1);
                duration_merge.push_back(d2);
                duration_corr.push_back(d3);
#endif //-------------------------------------------------------------------------------------------
#if(CORR_FPGA == 1) //------------------------------------------------------------------------------

                float const d4 = (float)(std::chrono::duration_cast<std::chrono::nanoseconds>(t4-t3).count());
                duration_collect.push_back(d4);
#endif //-------------------------------------------------------------------------------------------                
            }
        } else {
            //std::cout << "\e[1mSyncing ...\e[0m" << std::endl;
            iqp->ibvSync(mstr);
        }
    }  
    
    std::cout << std::endl;

    // Done with Communication
    if (mstr) {
        iqp->sendAck(1);
        iqp->closeAck();
    } else {
        iqp->readAck();
        iqp->closeConnection();
    }


    // Handke Measurements
    int measurements = 1;
    std::sort(duration_all.begin(),duration_all.end());

#if(CORR_SW == 1) //-------------------------------------------------------------------------------
    
    vector_of_M3.clear();

    std::sort(duration_collect.begin(),duration_collect.end());
    std::sort(duration_merge.begin(),duration_merge.end());
    std::sort(duration_corr.begin(),duration_corr.end());

    measurements = 4;
#endif //-------------------------------------------------------------------------------------------
#if(CORR_FPGA == 1) //------------------------------------------------------------------------------

    std::sort(duration_collect.begin(),duration_collect.end());
    measurements = 2;
#endif //-------------------------------------------------------------------------------------------    

    std::vector<float> measure_throughput;
    std::vector<float> measure_latency;
        
    for(unsigned i=0; i<measurements; i++) {
        for(unsigned r=0; r<n_reps; r++) {
            if(i==0) {
                measure_throughput.push_back(2*totalTransferSize/duration_all[r]);
                measure_latency = duration_all;
            }
            else{
                if(i==1) measure_latency = duration_collect;
                if(i==2) measure_latency = duration_merge;
                if(i==3) measure_latency = duration_corr;
            }
        }

        std::sort(measure_throughput.begin(),measure_throughput.end());
        std::sort(measure_latency.begin(),measure_latency.end());
    
        float min_lat = measure_latency[0]/1000;
        float max_lat = measure_latency[n_reps-1]/1000;
        float min_thr = measure_throughput[0];
        float max_thr = measure_throughput[n_reps-1];

        float p25_lat = 0.0;
        float p50_lat = 0.0;
        float p75_lat = 0.0;

        float p25_thr = 0.0;
        float p50_thr = 0.0;
        float p75_thr = 0.0;

        if(n_reps>=4) {
            p25_lat = measure_latency[(n_reps/4)-1]/1000;
            p50_lat = measure_latency[(n_reps/2)-1]/1000;
            p75_lat = measure_latency[(n_reps*3)/4-1]/1000;
    
            p25_thr = measure_throughput[(n_reps/4)-1];
            p50_thr = measure_throughput[(n_reps/2)-1];
            p75_thr = measure_throughput[(n_reps*3)/4-1];
        }

        float p1_lat  = 0.0;
        float p5_lat  = 0.0;
        float p95_lat = 0.0;
        float p99_lat = 0.0;

        float p1_thr  = 0.0;
        float p5_thr  = 0.0;
        float p95_thr = 0.0;
        float p99_thr = 0.0;

        if(n_reps >= 100) {
            p1_lat  = measure_latency[((n_reps)/100)-1]/1000;
            p5_lat  = measure_latency[((n_reps*5)/100)-1]/1000;
            p95_lat = measure_latency[((n_reps*95)/100)-1]/1000;
            p99_lat = measure_latency[((n_reps*99)/100)-1]/1000;

            p1_thr  = measure_throughput[((n_reps)/100)-1];
            p5_thr  = measure_throughput[((n_reps*5)/100)-1];
            p95_thr = measure_throughput[((n_reps*95)/100)-1];
            p99_thr = measure_throughput[((n_reps*99)/100)-1];
        }

        measure_latency.clear();
        measure_throughput.clear();

        FILE *file;
        std::string name = "nothing_defined.txt";

        //Open input file
#if(CORR_NONE==1) //---------------------------------------------------------
        name = "amnes_lat_rdma_only_" + std::to_string(mstr) + ".txt";
#endif //--------------------------------------------------------------------
#if(CORR_SW==1) //-----------------------------------------------------------
        name = "amnes_lat_rdma_corr_sw_" + std::to_string(mstr) + "_" + std::to_string(i) + "_" + std::to_string(numThreads)+ ".txt";
#endif //---------------------------------------------------------------------
#if(CORR_FPGA==1) //-----------------------------------------------------------
        name = "amnes_lat_rdma_corr_fpga_" + std::to_string(mstr) + "_" + std::to_string(i) + ".txt";
#endif //---------------------------------------------------------------------

        file = fopen(name.c_str(), "a");

        //input files size
        unsigned int file_size_result = get_filesize(file);
        if (file_size_result == 0)
            fprintf(file, "totalTransferSize transferSize streams streamitems repeat min max p1 p5 p25 p50 p75 p95 p99 \n");
 
        fseek(file, file_size_result, SEEK_END); 
        fprintf(file,"%u %u %u %u %u %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f \n",
            totalTransferSize, transferSize, streams, streamItems, n_reps, 
            min_lat, max_lat, p1_lat, p5_lat, p25_lat, p50_lat, p75_lat, p95_lat, p99_lat);
        fclose(file);

#if(CORR_NONE==1) //---------------------------------------------------------------
        name = "amnes_thr_rdma_only_" + std::to_string(mstr) + ".txt";
#endif //--------------------------------------------------------------------------
#if(CORR_SW==1 && i==0) //---------------------------------------------------------
        name = "amnes_thr_rdma_corr_sw_" + std::to_string(mstr) + ".txt";
#endif //--------------------------------------------------------------------------
#if(CORR_FPGA==1 && i==0) //---------------------------------------------------------
        name = "amnes_thr_rdma_corr_fpga_" + std::to_string(mstr) + ".txt";
#endif //--------------------------------------------------------------------------

        if(i==0) {
            file = fopen(name.c_str(), "a");
            //input files size
            file_size_result = get_filesize(file);
            if (file_size_result == 0)
                fprintf(file, "totalTransferSize transferSize streams streamitems repeat min max p1 p5 p25 p50 p75 p95 p99 \n");
    
            fseek(file, file_size_result, SEEK_END); 
            fprintf(file,"%u %u %u %u %u %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f \n",
            totalTransferSize, transferSize, streams, streamItems, n_reps, 
            min_thr, max_thr, p1_thr, p5_thr, p25_thr, p50_thr, p75_thr, p95_thr, p99_thr);
        
            fclose(file);
        }
    }

    duration_all.clear();
    duration_collect.clear();
    duration_merge.clear();
    duration_corr.clear();


    return EXIT_SUCCESS;
}