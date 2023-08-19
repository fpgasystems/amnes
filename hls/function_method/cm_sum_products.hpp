#ifndef CM_SUM_PRODUCTS_HPP
#define CM_SUM_PRODUCTS_HPP

#include <ap_int.h>

#include "../axi_utils.hpp"
#include "../memory/mem_utils.hpp"

template<unsigned line_width, unsigned data_width, unsigned result_width, unsigned reg_width>
void cm_sum_products(
     hls::stream<net_axis<line_width> >&   dataIn,
     hls::stream<net_axis<line_width> >&   sumPOut,
     ap_uint<reg_width>                    regSampleSize)
{
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    net_axis<line_width> lineData;
    ap_uint<data_width>  wordData[line_width/data_width];
   
    unsigned N      = line_width/data_width;
    unsigned M      = (line_width/data_width*(line_width/data_width-1))/2 - (int)((line_width/data_width*(line_width/data_width-1))/2/(line_width/result_width))*(line_width/result_width);
    
    unsigned noProd = (line_width/data_width*(line_width/data_width-1))/2;
    unsigned shift  = 5;

    static ap_uint<result_width> currentSumP[(line_width/data_width*(line_width/data_width-1))/2];        
    #pragma HLS DEPENDENCE variable=currentSumP inter false 
    #pragma HLS array_partition variable=currentSumP complete

    static ap_uint<reg_width> currentSampleCounter  = 0;
    static ap_uint<reg_width> productCounter = 0;

    ap_uint<line_width> sumPResult;
    ap_uint<line_width/8> keep;
    ap_uint<1> last;

    enum fsmStateType {COMPUTE, WRITE, REINIT};
    static fsmStateType state = COMPUTE;

    switch (state)
    {
      case COMPUTE:

        if(!dataIn.empty())
        {
          dataIn.read(lineData);
        
          for(unsigned i=0; i<N; i++)
          {
            #pragma HLS UNROLL
            wordData[i] = (ap_uint<data_width>)lineData.data((i+1)*data_width-1, i*data_width);
          }

          for(unsigned i=0; i<N; i++)
          {
            
            #pragma HLS UNROLL 
            for(unsigned j=0; j<N; j++)
            { 
              #pragma HLS UNROLL
              if(i<j)
              {
                // unsigned index = i<<4-(i*(i+1))>>1+j-i-1; 
                unsigned index = i*16-(i*(i+1))/2+j-i-1; 
                ap_uint<result_width> value = ((ap_uint<result_width>)wordData[i])*((ap_uint<result_width>)wordData[j]);
                currentSumP[index] += value;
              }
            }             
          }

          currentSampleCounter++;
          if(currentSampleCounter == regSampleSize)
          { 
            currentSampleCounter = 0; 
            state = WRITE;
          }
        }
      break;

      case WRITE:

          keep = 0x0;  

          if((productCounter+N) < (ap_uint<reg_width>)(noProd))
          {  
              for(unsigned i=0; i<N; i++) 
              {
               #pragma HLS UNROLL
               unsigned idx = i + (unsigned)productCounter;
               sumPResult((i+1)*result_width-1,i*result_width)= currentSumP[idx];

                //std::cout<<"SUM P "<<i<<" "<< currentSumP[i] <<std::endl;
                //std::cout<<std::dec<<"product counter: "<<productCounter<<" index "<<idx<<std::endl;
              }
    
              keep = ~((ap_uint<line_width/8>)0x0);
              last = 0x0;

              productCounter+=N; 
          }
          else
          {
              
              for(unsigned i=0; i<M; i++) 
              {
               #pragma HLS UNROLL
               unsigned idx = i + (unsigned)productCounter;
               sumPResult((i+1)*result_width-1,i*result_width)= currentSumP[idx];

               keep((i+1)*result_width/8-1,i*result_width/8) = ~((ap_uint<result_width/8>)0x0);
              }
    
              last = 0x1;

              //std::cout<<"noProd: "<<noProd<<" productCounter: "<<productCounter<<" sum:"<<currentSumP[idx]<<std::endl;
             // std::cout<<"sum:" <<sumPResult<<" keep"<<keep<<std::endl;
              productCounter+=M;

          }
            
            sumPOut.write(net_axis<line_width>(sumPResult, keep, last));   

          if(last == 0x1)        
          {
            state = REINIT;
            productCounter = 0x0;
            std::cout<<"Move to REINIT state."<<std::endl;
          }
      break;

      case REINIT:
        //   for(unsigned i=0; i<N; i++)
        //    {
        //       #pragma HLS UNROLL
        //       currentSumP[i] = 0x0;
        //    }
        //    
        //    currentSampleCounter = 0x0;
        //    state = COMPUTE;
     break;

    } //switch
          
}       

#endif
