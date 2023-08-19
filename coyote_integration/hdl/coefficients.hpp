#ifndef COEFFICIENTS_HPP
#define COEFFICIENTS_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include <math.h>

template<
    typename TR,
    unsigned N,
    unsigned P
>
class COEFF {
    enum fsmStateType {FEED=0, COMPUTE, SETTLE1, SETTLE2, READOUT};
    fsmStateType state = FEED;

    TR mem_e[N]  = {0, };
    TR mem_sq[N] = {0, };
    TR mem_p[P]  = {0, };
    
    float coeff[P]  = {0, };
    float coeff_value = 0.0f;
    bool last  = false;
    bool write = false;

    unsigned counter    = 0;
    unsigned counter_rd = 0;
    unsigned row        = 0;
    unsigned col        = 1;
    unsigned index      = 0;

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
        unsigned         samples_number,
        TIXL &&tixl = TIXL(),     // TI.last
        TIXV &&tixv = TIXV(),     // TI.val, idx
        TOXL &&toxl = TOXL(),     // TO.last
        TOXV &&toxv = TOXV()      // TO.val
    ) {
        #pragma HLS DATAFLOW
    
        #pragma HLS ARRAY_PARTITION variable=mem_e complete       
        #pragma HLS ARRAY_PARTITION variable=mem_sq complete        
        #pragma HLS ARRAY_PARTITION variable=mem_p complete

        #pragma HLS ARRAY_PARTITION variable=coeff complete

        #pragma HLS DATA_PACK variable=dst

        acc_coeff(src, dst, samples_number, tixl, tixv, toxl, toxv);
    } // compute()

private:
        
        template<typename TI, typename TO, typename TIXL, typename TIXV, typename TOXL, typename TOXV>  
        void acc_coeff(hls::stream<TI> &src, hls::stream<TO> &dst, unsigned samples_number, TIXL &&tixl, TIXV &&tixv, TOXL &&toxl, TOXV &&toxv){
            #pragma HLS INLINE off
            #pragma HLS PIPELINE II=1
                    
            if(write){
                TO y {};
                toxl(y) = last;
                toxv(y) = coeff_value;
                dst.write(y);
                
                write = false;
            }

            TI x;

            switch(state){
                case FEED:     
                    if(src.read_nb(x)){
                        auto const val = tixv(x);

                        if(counter < N)
                            mem_e[counter] = val;
                        else 
                            if(counter < (2*N))
                                mem_sq[counter-N] = val;
                            else
                                mem_p[counter-(2*N)] = val;
                                   
                        counter++; 

                        if(tixl(x)){ 
                           state   = SETTLE1;
                           counter = 0;
                        }
                    }
                break;
                case SETTLE1:
                    state = SETTLE2;
                break;
                case SETTLE2:
                    state = COMPUTE;
                break;
                case COMPUTE:    

                    if(index<P){                        
                        //std::cout<<"Index:"<<index<<" Row:"<<row<<" Col:"<<col<<std::endl;

                        TR const sum_e1  = mem_e[row];
                        TR const sum_e2  = mem_e[col];

                        TR const sum_sq1 = mem_sq[row];
                        TR const sum_sq2 = mem_sq[col];

                        TR const sum_p   = mem_p[index];

                        TR const numerator   = samples_number*sum_p - sum_e1*sum_e2;
                        TR const denominator = (samples_number*sum_sq1 - sum_e1*sum_e1) * (samples_number*sum_sq2-sum_e2*sum_e2);
                                
                        float value = 0.0f;

                        if(denominator!=0)
                            value = (float)numerator * pow(denominator,-0.5);                
                        
                        //if(isnan(value))
                        //     value = 0.0f;

                        coeff[index] = value;

//                        if((value<-1 or value>1) and (numerator>denominator)){
//                            std::cout<<"EXT"<<std::endl;
//                            std::cout<<"sums: "<<sum_e1<<" "<<sum_e2<<" "<<sum_sq1<<" "<<sum_sq2<<" "<<sum_p<<std::endl;
//                            std::cout<<"numerator: "<<numerator<<" denominator:"<<denominator<<std::endl;
//                        }
//
//                        if(isnan(value)){
//                            std::cout<<"NAN"<<std::endl;
//                            std::cout<<" sums: "<<sum_e1<<" "<<sum_e2<<" "<<sum_sq1<<" "<<sum_sq2<<" "<<sum_p<<std::endl;
//                            std::cout<<"numerator: "<<numerator<<" denominator:"<<denominator<<std::endl;
//                        }

                        index++;

                        if(col == (N-1)){
                            row++;
                            col = row+1;
                        }
                        else
                            col++;   
                    }

                    if(row == N-1)
                        state = READOUT;
                break;

                case READOUT:
                    coeff_value = coeff[counter_rd];
                    last        = ((counter_rd+1) == P) ? true:false;
                    write       = true;

                    counter_rd++;

                    if(last){
                        state = FEED;
                        counter_rd = 0;     
                    } 
                break;
            }   
        } // acc_coeff()
 
}; // class COEFF

#endif