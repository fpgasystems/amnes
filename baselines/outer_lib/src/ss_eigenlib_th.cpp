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
#include <memory>
#include <iostream>
#include <iomanip>
#include <chrono>

#include <cstdlib>
#include <cmath>
#include <algorithm>

#include <vector>
#include <thread>

#include <fstream>
#include <cstring>
#include <climits>

#include <Eigen/Dense>
#include <Eigen/StdVector>
#include <Eigen/Core>

#include <stdio.h>
#include <stdlib.h>

typedef uint32_t data_t;
typedef uint8_t acc_t;

typedef __int128 int128_t;
typedef unsigned __int128 uint128_t;

typedef Eigen::Matrix<data_t, Eigen::Dynamic, Eigen::Dynamic> MatrixXdata;
typedef Eigen::Matrix<acc_t, Eigen::Dynamic, Eigen::Dynamic> MatrixXacc;

void get_pcc_coefficients(uint32_t streams, uint32_t* collectorAddr, float* pccCoeff){
        
        MatrixXacc* M = (MatrixXacc *) collectorAddr;

        unsigned row = 0;
        unsigned col = 1;

        unsigned const pccItems  = streams*(streams-1)>>1;
        unsigned const num_items = M->coeff(streams,streams);

        for(unsigned j=0; j<pccItems; j++){
            
            //int128_t numerator    = (int128_t)((num_items * this->m_sum_prds[j])-(this->m_sum_elms[row] * this->m_sum_elms[col]));
            //int128_t denominator1 = (int128_t)((num_items * this->m_sum_sqrs[row])-(this->m_sum_elms[row] * this->m_sum_elms[row]));
            //int128_t denominator2 = (int128_t)((num_items * this->m_sum_sqrs[col])-(this->m_sum_elms[col] * this->m_sum_elms[col]));

            int128_t numerator    = (int128_t)((num_items * (M->coeff(row,col)))-(M->coeff(row,streams) * (M->coeff(col,streams))));
            int128_t denominator1 = (int128_t)((num_items * (M->coeff(row,row)))-(M->coeff(row,streams) * (M->coeff(row,streams))));
            int128_t denominator2 = (int128_t)((num_items * (M->coeff(col,col)))-(M->coeff(col,streams) * (M->coeff(col,streams))));
            
            float corr = 0.0;
            
            //std::cout<<"P: "<<this->m_sum_prds[j]<<" S: "<<this->m_sum_elms[row]<<" SQ: "<<this->m_sum_sqrs[row]<<std::endl;
            if ((denominator1>0 && denominator2>0) || (denominator1<0 && denominator2<0))
                corr = (float)(numerator/sqrt(std::abs(denominator1))/sqrt(std::abs(denominator2)));
            
            pccCoeff[j] = (float)corr;
    
            if(col==(streams-1)){
                row++;
                col = row+1;
            }
            else {
                col++;
            }
        }
}

int main(int argc, char* argv[]) {

    Eigen::initParallel();

    // Validate and Capture Arguments
    if (argc < 6) {
        std::cout << "Usage: " << argv[0] << " <num_items> <num_streams> <databits> <num_threads> <repetitions> <filename> \n";
        return  1;
    }

    unsigned const num_streams = strtoul(argv[2], nullptr, 0);
    if(num_streams < 2) {
        std::cerr << "Correlation needs minimum 2 streams." << std::endl;
        return  1;
    }
     
    unsigned const per_block   = num_streams;
    unsigned const num_items = strtoul(argv[1], nullptr, 0);
    if(num_items%per_block != 0) {
        std::cerr << "Num_items must be a multiple of " << per_block << std::endl;
        return  1;
    }

    unsigned const databits = strtoul(argv[3], nullptr, 0);
    if((databits < 8) || (databits > 32)) {
        std::cerr << "Data representation between 8 and 32 bits." << std::endl;
        return  1;
    }
    
    unsigned repetitions = strtoul(argv[5], nullptr, 0);
    if(repetitions < 1) 
        repetitions = 1;
    
    std::string filename;
    std::string message;
    std::fstream source_file;

    if(argv[6] != NULL){

        filename = argv[6];
        
        source_file.open (filename, std::ios::in);
        if(source_file.fail()){
            std::cerr<<"Could not open the file.\n";
            return 1;
        }
        message = filename;
    }
    else{

        message  = "No filename. Locally generated values.";
        filename = "";
    }

    size_t const num_blocks    = num_items/per_block;
    unsigned const num_threads = (unsigned)std::min(num_blocks, (size_t)strtoul(argv[4], nullptr, 0));
    
    unsigned const num_cores = std::thread::hardware_concurrency();
     
    std::cout
        << "Items/Stream :" << num_items
        << " \nStreams      :" << num_streams
        << " \nDatabits     :" << databits
        << " \nNumber Blocks:" << num_blocks
        << " \nThreads      :" << num_threads << " (mod " << num_cores << " cores)" 
        << " \nRepetitions  :" << repetitions
        << " \nFilename     :" << message
        << " \n--------------" << std::endl;
     
    std::cout<<"Eigen Lib: "<<EIGEN_MAJOR_VERSION<<"."<<EIGEN_MINOR_VERSION<<std::endl;
    int rows = num_streams+1;
    
    MatrixXacc M3(rows,rows);
    MatrixXacc M3_collect(rows,rows);

    std::vector<MatrixXacc> vector_of_M3;

    for(unsigned i=0; i<num_threads; i++) vector_of_M3.push_back(M3);
     
    acc_t* input = static_cast<acc_t *>(aligned_alloc(64, sizeof(acc_t)*num_items*num_streams));
    acc_t v = 0;

    for(unsigned i=0; i<num_items; i++) {
        for(unsigned j=0; j<num_streams; j++)
            input[i*num_streams + j] = v;
        v++;
    }

    std::vector<float> durations_collect;
    std::vector<float> durations_merge;
    std::vector<float> durations_pcc;
    
    //acc_t *ptr = M1.data();
    //for(unsigned t=0; t<35;t++){
    //    std::cout<<*(ptr+t)<<" ";
    //    if (t%16==0) std::cout<<std::endl;
    //}
    float pccCoeff[num_streams*(num_streams-1)>>1];

    for(unsigned r=0; r<repetitions; r++) {

        M3_collect.setZero();

        auto const t0 = std::chrono::system_clock::now();
        {
            std::vector<std::thread> threads;
            size_t ofs = 0;

            for(unsigned i=0; i<num_threads; i++) {
                
            size_t const nofs = (((i+1)*num_blocks)/num_threads)*per_block;     
            size_t const cnt  = nofs - ofs;

            threads.emplace_back([i, pdata=&input[ofs*num_streams], num_streams, cnt, pM3=&vector_of_M3[i] ](){
            
                MatrixXacc M1_th = Eigen::Map<MatrixXacc>(pdata, num_streams, cnt);
                M1_th.conservativeResize(num_streams+1, cnt);
                
                M1_th.row(num_streams).fill(1);
        
                MatrixXacc* M3_th = pM3;
               *M3_th = M1_th * M1_th.transpose();
            });

            cpu_set_t cpus;
            CPU_ZERO(&cpus);
            CPU_SET((i)%num_cores, &cpus); //i<<1
            pthread_setaffinity_np(threads.back().native_handle(), sizeof(cpus), &cpus);    
                
            ofs = nofs;
        }  

            // Wait for all to finish
            for(std::thread& t : threads) t.join();
    }
        
        // Sum all partial sums
        auto const t1 = std::chrono::system_clock::now();

        for(unsigned i=0; i<num_threads; i++)
            M3_collect += vector_of_M3[i];
    
        auto const t2 = std::chrono::system_clock::now();

        get_pcc_coefficients(num_streams, (uint32_t*) &M3_collect, pccCoeff);

        auto const t3 = std::chrono::system_clock::now();

        float const d0 = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        float const d1 = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t0).count();
        float const d2 = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t0).count();

        durations_collect.push_back(d0); 
        durations_merge.push_back(d1);
        durations_pcc.push_back(d2);
    }
   
    vector_of_M3.clear();
    
    //std::cout<<"M3(0,0): "<<M3_collect(0,0)<<std::endl;
    //std::cout<<M3_collect<<std::endl;

    //for(uint32_t i=0; i<120;i++)
    //    std::cout<<pccCoeff[i]<<" ";
    //std::cout<<std::endl;
    //Get stats
    //collect-throughtput
    std::vector<float> th_collect;
    std::vector<float> compute_duration;

    for(unsigned i=0; i<3; i++) {
        for(unsigned r=0; r<repetitions; r++) {
            if(i==0){
                th_collect.push_back((sizeof(data_t)*num_items*num_streams)/durations_collect[r]);
                compute_duration = durations_collect;
            }
            
            if(i==1){
                th_collect.push_back((sizeof(data_t)*num_items*num_streams)/durations_merge[r]);    
                compute_duration = durations_merge;
            }

            if(i==2){
                th_collect.push_back((sizeof(data_t)*num_items*num_streams)/durations_pcc[r]);    
                compute_duration = durations_pcc;
            }
        }

        std::sort(th_collect.begin(),th_collect.end());
        std::sort(compute_duration.begin(), compute_duration.end());
    
        float min = th_collect[0];
        float max = th_collect[repetitions-1];

        float p25 = 0.0;
        float p50 = 0.0;
        float p75 = 0.0;

        if(repetitions>=4){
            p25 = th_collect[(repetitions/4)-1];
            p50 = th_collect[(repetitions/2)-1];
            p75 = th_collect[(repetitions*3)/4-1];
        }
    
        float p1  = 0.0;
        float p5  = 0.0;
        float p95 = 0.0;
        float p99 = 0.0;
        float iqr = p75 - p25;
    
        float lower_iqr = p25 - (1.5 * iqr);
        float upper_iqr = p75 + (1.5 * iqr);
        if (repetitions >= 100) {
            p1  = th_collect[((repetitions)/100)-1];
            p5  = th_collect[((repetitions*5)/100)-1];
            p95 = th_collect[((repetitions*95)/100)-1];
            p99 = th_collect[((repetitions*99)/100)-1];
        }
        
        th_collect.clear();

        std::fstream file;
        
        if(i==0){
            std::string name = "ss_tput_cllct_"+std::to_string(num_streams)+"_"+std::to_string(databits)+"_pth.txt";
            file.open (name, std::ios::app);         
        }
         
        if(i==1) {
            std::string name = "ss_tput_merge_"+std::to_string(num_streams)+"_"+std::to_string(databits)+"_pth.txt";
            file.open (name, std::ios::app);
        }

        if(i==2) {
            std::string name = "ss_tput_pcc_"+std::to_string(num_streams)+"_"+std::to_string(databits)+"_pth.txt";
            file.open (name, std::ios::app);
        }

        file.seekg (0, std::ios::end);
        int length = file.tellg();

        if(length == 0)
            file<<"items num_streams databits threads repet min max p1 p5 p25 p50 p75 p95 p99 iqr liqr uiqr dmin dmax"<<std::endl; 
    
        file<< num_items <<" "<< num_streams<<" "<<databits<<" "<<num_threads <<" "
            << repetitions <<" "<< std::setprecision(5) 
            << min <<" "<< max <<" "
            << p1 <<" "<< p5 <<" "<< p25 <<" "
            << p50 <<" "<< p75 <<" "<< p95 <<" "<<p99<<" "
            << iqr <<" "<< lower_iqr <<" "
            << upper_iqr<<" "
            << compute_duration[0]<<" "
            << compute_duration[repetitions-1]<<std::endl;
    
        file.close();
    }

    durations_collect.clear();
    durations_merge.clear();
    
    return  0;
}
