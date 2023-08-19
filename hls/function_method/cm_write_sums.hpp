#ifndef CM_WRITE_SUMS_HPP
#define CM_WRITE_SUMS_HPP

#include "../axi_utils.hpp"
#include "../memory/mem_utils.hpp"

template<int line_width, int data_width, int result_width, int mem_width, int reg_width>
void cm_write_sums( 
     hls::stream<net_axis<line_width> > & sumElements, 
     hls::stream<net_axis<line_width> > & sumSquares,     
     hls::stream<net_axis<line_width> > & sumProducts,     
     hls::stream<net_axis<mem_width> >  & m_axis_write_data)
{
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off
    
    net_axis<line_width> currentSum;
    net_axis<mem_width>  dataOut;

    unsigned N = line_width/data_width;
    //unsigned sums = N/2+2;
    
    static unsigned i     = 0;
    static unsigned write = 0;
    

    if(!sumElements.empty() && i==0)
    {
      sumElements.read(currentSum);
      write = 1;
    }
    
    if(!sumSquares.empty() && i==1)
    {
      sumSquares.read(currentSum);
      write = 1;
    }
  
    if(!sumProducts.empty() && i>1)
    {
      sumProducts.read(currentSum);
      write = 1;
    }

    if(write==1)
    {
      dataOut.data = (ap_uint<mem_width>)currentSum.data;
      dataOut.keep = currentSum.keep;
      dataOut.last = currentSum.last;

      m_axis_write_data.write(net_axis<mem_width>(currentSum.data, currentSum.keep, currentSum.last));  
      i++;
      write = 0;
    }

  //  if((i+1)==sums)
  //    i = 0;
}

#endif