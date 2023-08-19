/**
 * Copyright (c) 2022, Systems Group, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef AMNES_HPP
#define AMNES_HPP

#include <cstdint>
#include <memory>
#include <functional>
#include <vector>
#include <cmath>

#ifndef DATA
    typedef uint32_t data_t;
    typedef uint64_t acc_t;

    #define M_STREAMS 16
#else
    #if DATA == 8
        typedef uint8_t data_t;
        typedef uint32_t acc_t;

        #define M_STREAMS 64
    #endif
    
    #if DATA == 16
        typedef uint16_t data_t;
        typedef uint32_t acc_t;

        #define M_STREAMS 32
    #endif
    
    #if DATA == 32
        typedef uint32_t data_t;
        typedef uint64_t acc_t;

        #define M_STREAMS 16
    #endif
#endif

#define CACHE_LINE_SIZE 64

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

template <typename TD, typename TR>
class AmnesCollector {
    
    unsigned const m_streams;
    //sum of elements
    std::unique_ptr<TR[]> m_sum_elms;
    //sum of squares
    std::unique_ptr<TR[]> m_sum_sqrs;
    //sum of products
    std::unique_ptr<TR[]> m_sum_prds;

public:
    AmnesCollector(unsigned const streams)
    : m_streams(streams),
      m_sum_elms(new TR[streams]()),
      m_sum_sqrs(new TR[streams]()),
      m_sum_prds(new TR[streams*(streams-1)>>1]()) {}
    
    AmnesCollector(AmnesCollector&& o)
    : m_streams(o.m_streams),
      m_sum_elms(std::move(o.m_sum_elms)),
      m_sum_sqrs(std::move(o.m_sum_sqrs)),
      m_sum_prds(std::move(o.m_sum_prds)) {}

    ~AmnesCollector() {}

public:
    void collect(TD const *data, size_t num_items){
    
	unsigned const M_streams  = this->m_streams;
    
        for(size_t i=0; i<num_items; i++){

            unsigned p = 0;
            for(unsigned j=0; j<M_streams; j++){
                
                TD lvalue  = *(data+M_streams*i+j);  
                
                this->m_sum_elms[j] += lvalue; 
                this->m_sum_sqrs[j] += lvalue * lvalue;

                for(unsigned k=j+1; k<M_streams; k++){

                    TD plvalue = *(data+M_streams*i+k);
                    this->m_sum_prds[p] += lvalue * plvalue;

                    p++;
                }
            }
        }
    }

private:
    void merge0(AmnesCollector const& other){

	unsigned const M_streams  = this->m_streams;

        unsigned p = 0;
        for(unsigned  i = 0; i<M_streams; i++){
            
            TR *const ref_sum_elms = &this->m_sum_elms[i];
            TR *const ref_sum_sqrs = &this->m_sum_sqrs[i];
    
            TR const mrg_sum_elms = other.m_sum_elms[i];
            TR const mrg_sum_sqrs = other.m_sum_sqrs[i];
            
            //std::cout<<"Sums:"<<*ref_sum_elms<<" "<<mrg_sum_elms<<std::endl;
            //std::cout<<"Sqrs:"<<*ref_sum_sqrs<<" "<<mrg_sum_sqrs<<std::endl;
    
            *ref_sum_elms += mrg_sum_elms;
            *ref_sum_sqrs += mrg_sum_sqrs;

            for(unsigned k=i+1; k<M_streams; k++){

                TR *const ref_sum_prds = &this->m_sum_prds[p];
                TR  const mrg_sum_prds = other.m_sum_prds[p];
                *ref_sum_prds += mrg_sum_prds;

                p++;
            }
        }
    }

public:
    AmnesCollector& merge(AmnesCollector const& other) {
        merge0(other);
        return *this;
    }

public:
    void get_pcc_coefficients(size_t num_items, std::vector<float> &pcc_coeff){
        
        unsigned const M_streams  = this->m_streams;
        unsigned const M_products = this->m_streams*(this->m_streams-1)>>1;
    
        unsigned row = 0;
        unsigned col = 1;
    
        for(unsigned j=0; j<M_products; j++){
            
            int128_t numerator    = (int128_t)((num_items * this->m_sum_prds[j])-(this->m_sum_elms[row] * this->m_sum_elms[col]));
            int128_t denominator1 = (int128_t)((num_items * this->m_sum_sqrs[row])-(this->m_sum_elms[row] * this->m_sum_elms[row]));
            int128_t denominator2 = (int128_t)((num_items * this->m_sum_sqrs[col])-(this->m_sum_elms[col] * this->m_sum_elms[col]));
            
            float corr = 0.0;
            
            //std::cout<<"P: "<<this->m_sum_prds[j]<<" S: "<<this->m_sum_elms[row]<<" SQ: "<<this->m_sum_sqrs[row]<<std::endl;
            if ((denominator1>0 && denominator2>0) || (denominator1<0 && denominator2<0))
                corr = (float)(numerator/sqrt(std::abs(denominator1))/sqrt(std::abs(denominator2)));
            
            pcc_coeff[j] = (float)corr;
    
            if(col==(M_streams-1)){
                row++;
                col = row+1;
            }
            else {
                col++;
            }
        }
    }

public:
    void clean(){
        
	unsigned const M_streams  = this->m_streams;
        unsigned const M_products = this->m_streams*(this->m_streams-1)>>1;
   
        for(unsigned i=0; i<M_streams; i++){
            this->m_sum_elms[i] = 0;
            this->m_sum_sqrs[i] = 0;
        }
    
        for(unsigned i=0; i<M_products; i++)
            this->m_sum_prds[i] = 0;
    }

};
#endif
