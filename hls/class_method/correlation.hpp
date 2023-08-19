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

        accu += value;
    } // accumulate()

public:
    TR read_accumulator() {

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

        TR const value_sq = value;
        accu += value_sq * value_sq;

    } // accumulate_sq()

public:
    TR read_accumulator() {
		#pragma HLS INLINE

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
    TR  accu1[P/2];   // = {(TR)0.0, };
    TR  accu2[P/2];   // = {(TR)0.0, };

    unsigned output_index = 0;

    // Consuming data or delivering the sums of products
    bool returnval = false;

public:
    template<typename TV>
    bool process(TV (&value)[N], TR &val, bool product_state){
		#pragma HLS INLINE

		#pragma HLS ARRAY_PARTITION variable=accu1 type=complete
		#pragma HLS BIND_STORAGE variable=accu1 type=RAM_S2P impl=lutram

		#pragma HLS ARRAY_PARTITION variable=accu2 type=complete
		#pragma HLS BIND_STORAGE variable=accu2 type=RAM_S2P impl=lutram

    	#pragma HLS DEPENDENCE type=inter variable=accu1 dependent=false
    	#pragma HLS DEPENDENCE type=inter variable=accu2 dependent=false

    	TR interim_val;

    	if(product_state == true){

    		unsigned row = 0;
    		unsigned col = 1;

    		for(unsigned i=0; i<P; i++){

				#pragma HLS UNROLL

    			TR value_1 = value[row];
    			TR value_2 = value[col];
            
    			TR prod = value_1 * value_2;

    			if(i<P/2)
    				accu1[i] += prod;
    			else
    				accu2[i-P/2] += prod;

    			if((col+1) == N){

    				row++;
    				col = row+1;
    			}
    			else{
    				col++;
    			}
    		}

    		returnval = false;

    	}else{

            if(output_index<P/2){

                interim_val = accu1[output_index];
                accu1[output_index] = (TR)0.0;
            }
            else{

                interim_val = accu2[output_index-P/2];
                accu2[output_index-P/2] = (TR)0.0;
            }

            val = interim_val;
            returnval = false;

            output_index++;

            if(output_index == P) {

                output_index = 0;
                returnval    = true;
            }

    	 }

    	 return returnval;

    } // process()

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

        #pragma HLS ARRAY_PARTITION variable=sums_elements type=complete
		#pragma HLS BIND_STORAGE variable=sums_elements type=RAM_S2P impl=lutram
		#pragma HLS DEPENDENCE type=inter variable=sums_elements dependent=false

        #pragma HLS ARRAY_PARTITION variable=sums_squares type=complete
		#pragma HLS BIND_STORAGE variable=sums_squares type=RAM_S2P impl=lutram
		#pragma HLS DEPENDENCE type=inter variable=sums_squares dependent=false

    	#pragma HLS AGGREGATE variable=dst compact=bit

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
                    sums_products.process(values,top_result, true);
                
                    if(tixl(x)){

                        samples_number = samples_counter;
                        state = READOUT_S;                         
                    }
                }
                last = false;
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

            	decltype(tixv(x,0)) values[N];
                last  = sums_products.process(values, top_result, false);
                write = true;

                if(last){

                    samples_counter = 0;
                    samples_number  = 0;
                    
                    state = COMPUTE;
                }

            break;
        }
        
    } // accumulate()

}; // class CORR

#endif
