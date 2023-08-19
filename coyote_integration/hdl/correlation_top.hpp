#ifndef CORRELATION_TOP_HPP
#define CORRELATION_TOP_HPP

#include "user_hls_c0_0.hpp"
#include "correlation.hpp"
#include "coefficients.hpp"

unsigned const L = 512;       // Line width  
unsigned const D = 32;        // Data size
unsigned const R = 64;        // Result size
unsigned const N = L/D;       // Number of input lanes
unsigned const P = N*(N-1)/2; // Number of products

using result_data_t = ap_int<R>; // ap_ufixed<32,21> float ap_int<R>; ap_uint<R>; 
using data_t        = ap_int<D>; // ap_ufixed<16,2>  float ap_int<D>; ap_uint<D>;
using input_t       = flit_v_t<ap_int<L>>; //ap_int<L>>; // Input data type
using result_t      = flit_v_t<result_data_t>; //ap_int<R>>;
using coeff_t       = flit_v_t<float>; // float

void convert_data_in(
    hls::stream<axisrIntf> &src,
    hls::stream<input_t>   &dst,
    ap_uint<64> numberItems
){
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    axisrIntf x;
    static input_t y;
    static bool valid = false;
    static ap_uint<64> counter = 0;
   
    if(valid){
        dst.write(y);
        valid = false;
    } 

    if(src.read_nb(x)){
        y.val  = x.tdata;
        y.last = (counter+1==numberItems);
        counter++;
        valid  = true;
    }

    if(counter==numberItems)
        counter = 0;

} // convert_data_in()

void correlation_top(
    hls::stream<input_t>  &dataIn,
    hls::stream<coeff_t>  &dataOut
) {
    #pragma HLS DATAFLOW disable_start_propagation
    #pragma HLS INLINE off
    #pragma HLS INTERFACE ap_ctrl_none port=return
    
    static hls::stream<result_t> sums;

    static CORR<result_data_t,N, P>   accumulators;
    static COEFF<result_data_t, N, P> coefficients;
    
    static unsigned samples;

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

} // correlation_top()

void convert_data_out(
    hls::stream<coeff_t>   &src,
    hls::stream<axisrIntf> &dst
){
    #pragma HLS INLINE off
    #pragma HLS PIPELINE II=1

    static coeff_t x;
    static axisrIntf y;
    static bool valid = false;
    static unsigned counter = 0;
    
    if(valid){
        y.tlast = x.last;
        y.tdata = x.val;
        y.tkeep = 0xFFFFFFFF;

        dst.write(y);
        
        if(x.last)
            counter = 0;

        valid = false;
    }
    
    if(src.read_nb(x)){
        valid = true;
        counter++;
    }

} // convert_data_out()

#endif
