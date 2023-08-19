#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <math.h>

#include "../stream.hpp"
#include "../bit_utils.hpp"
#include "../axi_utils.hpp"
#include "../mem_utils.hpp"

unsigned const L = 512;       // Line width  
unsigned const D = 32;         // Data size
unsigned const R = 64;        // Result size
unsigned const N = L/D;       // Number of input lanes
unsigned const P = N*(N-1)/2; // Number of products
unsigned const DEST = 4;      // Destination bits

using result_data_t = ap_uint<R>; // ap_ufixed<32,21> float ap_int<R>; ap_uint<R>;  ap_int<R>
using data_t        = ap_uint<D>; // ap_ufixed<16,2>  float ap_int<D>; ap_uint<D>;  ap_int<D>
using input_t       = flit_v_t<ap_uint<L>>; //ap_int<L>>; // Input data type
using result_t      = flit_v_t<result_data_t>; //ap_int<R>>;
using coeff_t       = flit_v_t<float>; // float

void correlation_top(
    hls::stream<input_t>  &dataIn,
    hls::stream<coeff_t>  &dataOut
);

// Infrastructure Wrapper
void correlation(
    hls::stream<ap_axiu<L,0,0,0>>  &s_axis_input_tuple,
    hls::stream<ap_axiu<96,0,0,0>> &m_axis_write_cmd,
    hls::stream<ap_axiu<D,0,0,0>>  &m_axis_write_data,
    ap_uint<64>                    regBaseAddr
);
