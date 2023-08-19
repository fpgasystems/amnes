#ifndef CORRELATIONHPP
#define CORRELATIONHPP

#include <Eigen/Dense>
#include <Eigen/StdVector>
#include <Eigen/Core>
#include <cmath>
#include <cstdint>

typedef uint32_t data_t;
typedef uint32_t acc_t;

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

typedef Eigen::Matrix<uint32_t, Eigen::Dynamic, Eigen::Dynamic> MatrixXdata;
typedef Eigen::Matrix<uint32_t, Eigen::Dynamic, Eigen::Dynamic> MatrixXacc;

typedef struct {
    uint32_t  streamNumber;
    uint32_t  streamItems;
    uint32_t* pIntermResult;
} corr_thread_args_t;

typedef struct {
    corr_thread_args_t* corr_thread;
    uint32_t  memSize;
    uint32_t* startAddr;
    uint32_t  threads;
    uint32_t  chunk;
} corr_chunk_args_t;

void* corr_chunk(void* args) {

   corr_chunk_args_t* a = (corr_chunk_args_t*)args;

   uint32_t memSizeB = a->memSize;
   uint32_t streams  = a->corr_thread->streamNumber;
   uint32_t iterationsPerThread = memSizeB/a->chunk/a->threads;

   //std::cout<<"TEST iteration per thread: "<<iterationsPerThread<<std::endl;
   //std::cout<<"TEST start address: "<<a->startAddr<<std::endl;  
 
   for (uint32_t i = 0; i<iterationsPerThread; i++) {
      
        uint32_t* threadStartAddr = a->startAddr + i*a->threads*a->chunk/sizeof(uint32_t);    
        volatile uint32_t* pollAddr = threadStartAddr + a->chunk/sizeof(uint32_t) - 1;
        
        a->corr_thread->streamItems = a->chunk/streams/sizeof(uint32_t);
        //std::cout<<"Stream Items: "<< a->corr_thread->streamItems<<std::endl;

        while(*pollAddr==0);
        //std::cout<<"Chunk received at: "<<(void*)threadStartAddr<<std::endl;
        //std::cout<<"         pollAddr: "<<(void*)pollAddr<<std::endl;
        //std::cout<<"Value at pollAddr: "<<*(pollAddr)<<std::endl;

        // call eigen
        MatrixXacc M1_th = Eigen::Map<MatrixXacc>(threadStartAddr, streams, a->corr_thread->streamItems);
        M1_th.conservativeResize(streams+1, a->corr_thread->streamItems);        
        M1_th.row(streams).fill(1);

        MatrixXacc* M3_th = (MatrixXacc *) a->corr_thread->pIntermResult;
        *M3_th += M1_th * M1_th.transpose();   
   }

   return NULL;
}

void get_pcc_coefficients(uint32_t streams, uint32_t* collectorAddr, float* pccCoeff){
        
        MatrixXacc* M = (MatrixXacc *) collectorAddr;

        unsigned row = 0;
        unsigned col = 1;

        unsigned const pccItems  = streams*(streams-1)>>1;
        unsigned const num_items = M->coeff(streams,streams);

        for(unsigned j=0; j<pccItems; j++){
            
            //int128_t numerator    = (int128_t)((num_items * this->m_sum_prds[j])-(this->m_sum_elms[row] * this->m_sum_elms[col]));
            //int128_t denominator1 = (int128_t)((num_items * this->m_sum_sqrs[row])-(this->m_sum_elms[row] * this->m_sum_elms[row]));
            //int128_t denominator2 = (int128_t)((num_items * this->m_sum_sqrs[col])-(this->m_sum_elms[col] * this->m_sum_elms[col]));

            int128_t numerator    = (int128_t)((num_items * (M->coeff(row,col)))-(M->coeff(row,streams) * (M->coeff(col,streams))));
            int128_t denominator1 = (int128_t)((num_items * (M->coeff(row,row)))-(M->coeff(row,streams) * (M->coeff(row,streams))));
            int128_t denominator2 = (int128_t)((num_items * (M->coeff(col,col)))-(M->coeff(col,streams) * (M->coeff(col,streams))));
            
            float corr = 0.0;
            
            //std::cout<<"P: "<<this->m_sum_prds[j]<<" S: "<<this->m_sum_elms[row]<<" SQ: "<<this->m_sum_sqrs[row]<<std::endl;
            if ((denominator1>0 && denominator2>0) || (denominator1<0 && denominator2<0))
                corr = (float)(numerator/sqrt(std::abs(denominator1))/sqrt(std::abs(denominator2)));
            
            pccCoeff[j] = (float)corr;
    
            if(col==(streams-1)){
                row++;
                col = row+1;
            }
            else {
                col++;
            }
        }
}

#endif