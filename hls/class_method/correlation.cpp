#include "correlation_parameters.hpp"
#include "correlation.hpp"
#include "coefficients.hpp"


// Global variable for convert_data_in (previous static)
bool valid_read = false;
ap_uint<64> counter_read = 0;
input_t y = {0,0};

static void convert_data_in(
    hls::stream<ap_axiu<L,0,0,DEST>> &src,
    hls::stream<input_t>             &dst,
	ap_uint<64> 				     numberItems
){
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

	ap_axiu<L,0,0,DEST> x;

    if(valid_read){

        dst.write(y);
        valid_read = false;
    } 

    if(src.read_nb(x)){

        y.val  = x.data;
        y.last = (counter_read+1==numberItems);
        counter_read++;
        valid_read  = true;
    }

    if(counter_read == numberItems)
    	counter_read = 0;

} // convert_data_in()

// Global variable for correlation_top
CORR<result_data_t,N, P>   accumulators;
COEFF<result_data_t, N, P> coefficients;

unsigned samples;

void correlation_top(
    hls::stream<input_t>  &dataIn,
    hls::stream<coeff_t>  &dataOut
) {

    #pragma HLS DATAFLOW

	static hls::stream<result_t> sums;

    accumulators.compute(
        dataIn, sums, samples,
        MemberLast(), [](input_t const& x, unsigned const idx)-> data_t{
            return x.val((idx+1)*D-1, idx*D);
        },
        MemberLast(), MemberVal()
    );

    coefficients.compute(
        sums, dataOut, samples,
        MemberLast(), MemberVal(),
        MemberLast(), MemberVal()
    );

} // top()

// Global variable for convert_data_out
bool valid_write = false;
ap_uint<64> counter_write = 0;
coeff_t x;

void convert_data_out(
    hls::stream<coeff_t>             &src,
    hls::stream<ap_axiu<L,0,0,DEST>> &dst
){
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    ap_axiu<L,0,0,DEST> y;

    if(valid_write){

        y.last = x.last;
        union { float f; uint32_t i; } conv = { .f = x.val};
        y.data = conv.i;
        y.keep = 0xFFFFFFFF;

        dst.write(y);

        valid_write = false;
    }

    if(src.read_nb(x)){
        valid_write = true;
    }

} // convert_data_out()

void design_user_hls_c0_0 (
    // Host streams
    hls::stream<ap_axiu<L,0,0,DEST>>& axis_host_sink,
    hls::stream<ap_axiu<L,0,0,DEST>>& axis_host_src,

    ap_uint<64> numberItems,
	ap_uint<64> vaddress,
	ap_uint<32> len,
	ap_uint<32> pid,

	ap_uint<64> &itemsReg,
	ap_uint<64> &vaddrReg,
	ap_uint<32> &lenReg,
	ap_uint<32> &pidReg
) {
    #pragma HLS DATAFLOW disable_start_propagation
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE axis register port=axis_host_sink name=s_axis_host_sink
    #pragma HLS INTERFACE axis register port=axis_host_src  name=m_axis_host_src

    #pragma HLS INTERFACE s_axilite port=return      bundle=control
    #pragma HLS INTERFACE s_axilite port=numberItems bundle=control
	#pragma HLS INTERFACE s_axilite port=vaddress    bundle=control
	#pragma HLS INTERFACE s_axilite port=len         bundle=control
	#pragma HLS INTERFACE s_axilite port=pid         bundle=control

    #pragma HLS INTERFACE mode=ap_none port=itemsReg
	#pragma HLS INTERFACE mode=ap_none port=vaddrReg
	#pragma HLS INTERFACE mode=ap_none port=lenReg
	#pragma HLS INTERFACE mode=ap_none port=pidReg

    //
    // User logic
    //
    static hls::stream<input_t> data_source;
    static hls::stream<coeff_t> result_values;
    ap_uint<64> items = numberItems;

    itemsReg = numberItems;
    vaddrReg = vaddress;
    lenReg   = len;
    pidReg   = pid;

    convert_data_in(axis_host_sink, data_source, items);
    correlation_top(data_source,result_values);
    convert_data_out(result_values, axis_host_src);
}


///// RDMA integration
static void write_results_memory(
    hls::stream<coeff_t>           &result_values,
    hls::stream<ap_axiu<96,0,0,0>> &m_axis_write_cmd,
    hls::stream<ap_axiu<D,0,0,0>>  &m_axis_write_data,
    ap_uint<64>                    regBaseAddr
) {
    #pragma HLS PIPELINE II=1
    #pragma HLS INLINE off

    static unsigned const  WORDS_PER_LINE   = L / 32;
    // static unsigned const  TOTAL_RES_WORDS  = CM_RES_LINES*WORDS_PER_LINE + 3;
    static coeff_t x;
    static bool valid = false;
    static unsigned counter = 0;

    static ap_axiu<96,0,0,0> y;

    if(valid){
        m_axis_write_data.write(ap_axiu<D,0,0,0>{x.val, ap_int<D/8>{-1}, x.last});
        if(counter==1){
        	y.data = 0; //{regBaseAddr, WORDS_PER_LINE};
            m_axis_write_cmd.write(y);
        }

        if(x.last)
            counter = 0;

        valid = false;
    }
    
    if(result_values.read_nb(x)){
        valid = true;
        counter++;
    }
} // write_results_memory()

// Infrastructure Wrapper
void correlation(
    hls::stream<ap_axiu<L,0,0,0>>  &s_axis_input_tuple,
    hls::stream<ap_axiu<96,0,0,0>> &m_axis_write_cmd,
    hls::stream<ap_axiu<D,0,0,0>>  &m_axis_write_data,
    ap_uint<64>                    regBaseAddr
){
    #pragma HLS INLINE off
    #pragma HLS DATAFLOW disable_start_propagation
    #pragma HLS INTERFACE ap_ctrl_none port=return

    #pragma HLS INTERFACE axis port=s_axis_input_tuple

    #pragma HLS INTERFACE axis port=m_axis_write_cmd

    #pragma HLS INTERFACE axis port=m_axis_write_data

    #pragma HLS INTERFACE ap_none register port=regBaseAddr
    
    static hls::stream<input_t> data_source;
    static hls::stream<coeff_t> result_values;

    //convert_data(s_axis_input_tuple, data_source);
    correlation_top(data_source,result_values);
    write_results_memory(result_values, m_axis_write_cmd, m_axis_write_data, regBaseAddr);
}// correlation()
