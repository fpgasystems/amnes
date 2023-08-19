#ifndef CORRELATION_HPP
#define CORRELATION_HPP

#include <algorithm>

//----- SUM OF ELEMENTS -----//
template<
    typename TR
>
class CORR_SUM {
    
    // accumulator to store the sum of elements
    TR accu = (TR)0.0;

public:
    template<typename TV>
    void accumulate(TV value){
        #pragma HLS INLINE

        accu += value;
    } // accumulate()

public:
    TR read_accumulator() {
        #pragma HLS INLINE

        TR const result_value = accu;
        accu = (TR)0.0;
        return  result_value;

    } // read_accumulator()

}; // CORR_SUM


//----- SUM OF SQUARES -----//
template<
    typename TR
>
class CORR_SUM_SQ {
    
    // accumulator to store the sum of squares
    TR  accu = (TR)0.0;

public:
    template<typename TV>
    void accumulate_sq(TV value){
        #pragma HLS INLINE
        #pragma HLS PIPELINE II=1

        TR const value_sq = value; 
        
        //TR val = accu;

        accu += value_sq * value_sq;

        //if(accu < val)
        //    std::cout<<"Overflow: "<<val<<" "<<accu<<std::endl;
    } // accumulate_sq()

public:
    TR read_accumulator() {
        #pragma HLS INLINE
        #pragma HLS PIPELINE II=1

        TR const result_value = accu;
        accu = (TR)0.0;
        return  result_value;

    } // read_accumulator()

}; // CORR_SUM_SQ


//----- SUM OF PRODUCTS -----//
template<
    typename TR,
    unsigned N,  // Number of streams
    unsigned P   // Number of products
>
class CORR_SUM_P {
    
    // array of accumulators to store the sum of products
    TR  accu[P] = {(TR)0.0, };
    unsigned output_index = 0;

public:
    template<typename TV>
    void process(TV (&value)[N]){
        #pragma HLS INLINE
        #pragma HLS PIPELINE II=1
       
        #pragma HLS ARRAY_PARTITION variable=accu complete
       
        for(unsigned i=0; i<N; i++){
            #pragma HLS UNROLL

            for(unsigned j=0; j<N; j++){
                #pragma HLS UNROLL

                if(i<j){
                    unsigned index = i*N -(i*(i+1))/2+j-i-1;
                    //std::cout<<"Index: "<<index<<std::endl;
                    TR value_1 = value[i];
                    TR value_2 = value[j];
                    accu[index] += value_1 * value_2; 
                }
            }
        }

    } // process()

public:
    bool read_accumulators(TR &val){
        #pragma HLS INLINE
       // #pragma HLS PIPELINE II=1

        val  = accu[output_index];
       
        accu[output_index] = (TR)0.0;
        output_index++;
        
        if(output_index==P) {    
            output_index = 0;
            return true;
        }
        
        return false;

    } // read_accumulators()

}; // CORR_SUM_P

//----- SUMS FOR CORRELATION----- //
template<
    typename TR, // type of the result,  
    unsigned N,  // Number of lanes
    unsigned P   // Number of products
>
class CORR {

    enum fsmStateType {COMPUTE, READOUT_S, READOUT_SQ, READOUT_P};
    fsmStateType state = COMPUTE;
    
    TR top_result;

    unsigned idx_s  = 0;
    unsigned idx_sq = 0;

    unsigned samples_counter = 0;
    
    bool last  = false;
    bool write = false;

    //- Structure -----------------------------------------------------------
    CORR_SUM<TR>       sums_elements[N];
    CORR_SUM_SQ<TR>    sums_squares[N];
    CORR_SUM_P<TR,N,P> sums_products;  

public:
    template<
        typename TI,
        typename TO,        
        typename TIXL = MemberLast,
        typename TIXV = MemberVal,
        typename TOXL = MemberLast,
        typename TOXV = MemberVal
    >
    void compute(
        hls::stream<TI>  &src,
        hls::stream<TO>  &dst,
        unsigned         &samples_number,
        TIXL &&tixl = TIXL(),     // TI.last
        TIXV &&tixv = TIXV(),     // TI.val, idx
        TOXL &&toxl = TOXL(),     // TO.last
        TOXV &&toxv = TOXV()      // TO.val
    ) {
        #pragma HLS DATAFLOW
        #pragma HLS ARRAY_PARTITION variable=sums_elements complete
        #pragma HLS ARRAY_PARTITION variable=sums_squares  complete
        #pragma HLS DATA_PACK variable=dst

        accumulate(src, dst, samples_number, tixl, tixv, toxl, toxv);
    } // compute()

private:

    template<typename TI, typename TO, typename TIXL, typename TIXV, typename TOXL, typename TOXV>
    void accumulate(hls::stream<TI> &src, hls::stream<TO> &dst, unsigned &samples_number, TIXL &&tixl, TIXV &&tixv, TOXL &&toxl, TOXV &&toxv) { 
        #pragma HLS PIPELINE II=1
       
        if(write){
            TO y {};
            toxl(y) = last;
            toxv(y) = top_result;
            dst.write(y);
            
            write = false;
        }

        TI x;
         
        switch(state){

            case COMPUTE:
                if(src.read_nb(x)){

                    decltype(tixv(x,0)) values[N];

                    for(unsigned i=0; i<N; i++){
                        #pragma HLS UNROLL

                        auto const val = tixv(x,i);
                        sums_elements[i].accumulate(val);
                        sums_squares[i].accumulate_sq(val);

                        values[i] = val;
                    }
                    
                    samples_counter++;
                    sums_products.process(values);
                
                    if(tixl(x)){
                        samples_number = samples_counter;
                        state = READOUT_S;                         
                    }
                }
            break;

            case READOUT_S:
                
                top_result = sums_elements[idx_s].read_accumulator();
                write      = true;
                
                idx_s++;
                 
                if(idx_s == N){
                    idx_s = 0;
                    
                    state = READOUT_SQ;
                }
            break;

            case READOUT_SQ:
                
                top_result = sums_squares[idx_sq].read_accumulator();
                write      = true;
                 
                idx_sq++;
                 
                if(idx_sq == N){
                    idx_sq = 0;
                    
                    state  = READOUT_P;
                }
            break;

            case READOUT_P:
                
                last  = sums_products.read_accumulators(top_result);
                write = true;

                if(last)
                    state = COMPUTE;
            break;
        }
        
    } // accumulate()

}; // class CORR

#endif