#ifndef CM_SUM_HPP
#define CM_SUM_HPP

#include <ap_int.h>

#include "../axi_utils.hpp"
#include "../memory/mem_utils.hpp"

template<int line_width, int data_width, int result_width, int reg_width>
void cm_sum(
     hls::stream<net_axis<line_width> >&   dataIn,
     hls::stream<net_axis<line_width> >&   dataOut,
     hls::stream<net_axis<line_width> >&   sumOut,
     ap_uint<reg_width>                    regSampleSize)
{
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    
    net_axis<line_width> lineData;
    ap_uint<data_width>  wordData[line_width/data_width];
    
    int N = line_width/data_width;

    static ap_uint<result_width> currentSum[line_width/data_width];    
    static ap_uint<reg_width>    currentSampleCounter = 0;

    
    if(!dataIn.empty())
    {
        dataIn.read(lineData);
        dataOut.write(lineData);
        
        for(int i=0; i<N; i++)
        {
            #pragma HLS UNROLL
            wordData[i] = (ap_uint<data_width>)lineData.data((i+1)*data_width-1, i*data_width);
        }

        for(int i=0; i<N; i++)
        {
            #pragma HLS UNROLL
            currentSum[i] += (ap_uint<result_width>)wordData[i];
        }
        
        currentSampleCounter++;

        if(currentSampleCounter == regSampleSize)
        {   
            ap_uint<line_width> sumResult = 0x0;

            for(int i=0; i<N; i++) 
            {
                #pragma HLS UNROLL
                sumResult((i+1)*result_width-1,i*result_width) = currentSum[i];
               
               // std::cout<<"SUM "<<i<<" "<< currentSum[i] <<std::endl;
            }

            ap_uint<line_width/8> keep = ~((ap_uint<line_width/8>)0x0);
           // std::cout<<"sum elements: "<<sumResult<<std::endl;
           // std::cout<<std::hex<<keep<<std::endl;
            sumOut.write(net_axis<line_width>(sumResult, keep, 0x0));

            for(int i=0; i<N; i++) 
            {
                #pragma HLS UNROLL
                currentSum[i]= 0x0;
            }
            
            currentSampleCounter = 0x0;           
        }    
    }
          
}       

#endif