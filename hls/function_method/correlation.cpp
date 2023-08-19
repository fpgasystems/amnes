#include "correlation.hpp"

void correlation(
    hls::stream<net_axis<LINEW> >& s_axis_data,
    hls::stream<net_axis<MEMW> >&  m_axis_write_data,
    ap_uint<REGW>                  regSampleSize,
    ap_uint<REGW>                  regBaseAddr)
{
    #pragma HLS DATAFLOW 
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE axis port=s_axis_data
    #pragma HLS INTERFACE axis port=m_axis_write_data

    //#pragma HLS INTERFACE axis port=m_axis_write_cmd
    //#pragma HLS DATA_PACK variable=m_axis_write_cmd

    #pragma HLS INTERFACE ap_stable register port=regSampleSize
    #pragma HLS INTERFACE ap_stable register port=regBaseAddr

      
    static hls::stream<net_axis<LINEW> > data_1_FIFO;
    #pragma HLS stream depth=8 variable=data_1_FIFO

    static hls::stream<net_axis<LINEW> > data_2_FIFO;
    #pragma HLS stream depth=8 variable=data_1_FIFO


    static hls::stream<net_axis<LINEW> > sums_FIFO;
    #pragma HLS stream depth=8 variable=sums_FIFO
    
    static hls::stream<net_axis<LINEW> > sumsS_FIFO;
    #pragma HLS stream depth=8 variable=sumsS_FIFO

    static hls::stream<net_axis<LINEW> > sumsP_FIFO;
    #pragma HLS stream depth=16 variable=sumsP_FIFO

    // Function templates   
    //  cm_sum: template<int line_width, int data_width, int result_width, int reg_width>
    //  cm_sums_squares: template<int line_width, int data_width, int result_width, int reg_width> 
    //  cm_sums_products: template<int line_width, int data_width, int result_width, int reg_width>
    //  write_sum: template<int result_width, int mem_width, int reg_width>
    
    cm_sum<LINEW, DATAW, RESW, REGW>(s_axis_data, data_1_FIFO, sums_FIFO, regSampleSize);     
   
    cm_sum_squares<LINEW, DATAW, RESW, REGW>(data_1_FIFO, data_2_FIFO, sumsS_FIFO, regSampleSize); 
                                                                                                                                                      
    cm_sum_products<LINEW, DATAW, RESW, REGW>(data_2_FIFO, sumsP_FIFO, regSampleSize);   

    cm_write_sums<LINEW, DATAW, RESW,MEMW,REGW>(sums_FIFO, sumsS_FIFO, sumsP_FIFO, m_axis_write_data);   
}
