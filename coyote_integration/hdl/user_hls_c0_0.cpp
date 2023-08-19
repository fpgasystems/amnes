#include "user_hls_c0_0.hpp"
#include "correlation_top.hpp"

/**
 * User logic
 *
 */
void design_user_hls_c0_0_top (
    // Host streams
    hls::stream<axisrIntf>& axis_host_sink,
    hls::stream<axisrIntf>& axis_host_src,

    ap_uint<64> numberItems,
    ap_uint<64> windowItems
) {
    #pragma HLS DATAFLOW disable_start_propagation
    #pragma HLS INTERFACE ap_ctrl_none port=return  

    #pragma HLS INTERFACE axis register port=axis_host_sink name=s_axis_host_sink
    #pragma HLS INTERFACE axis register port=axis_host_src name=m_axis_host_src

    
    #pragma HLS INTERFACE s_axilite port=return   bundle=control
    #pragma HLS INTERFACE s_axilite port=numberItems bundle=control
    #pragma HLS INTERFACE s_axilite port=windowItems bundle=control
    
    #pragma HLS INTERFACE ap_stable register port=numberItems

    //
    // User logic 
    //
    static hls::stream<input_t> data_source;
    static hls::stream<coeff_t> result_values;
    
    convert_data_in(axis_host_sink, data_source, numberItems);
    correlation_top(data_source,result_values);
    convert_data_out(result_values, axis_host_src);
}
