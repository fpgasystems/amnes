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

#include <stdio.h>
#include <stdlib.h>

typedef uint32_t data_t;
typedef uint64_t acc_t;

typedef Eigen::Matrix<data_t, Eigen::Dynamic, Eigen::Dynamic> MatrixXdata;
typedef Eigen::Matrix<acc_t, Eigen::Dynamic, Eigen::Dynamic> MatrixXacc;

int main(int argc, char* argv[]) {

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
    
    Eigen::setNbThreads(num_threads);

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
    unsigned no_threads_Eigen = (unsigned) Eigen::nbThreads();

    std::cout<<"Eigen Ths: "<<no_threads_Eigen<<std::endl;

    int rows = num_streams+1;
    int cols = (int)num_items;
    
    MatrixXacc M1(rows,cols);
    MatrixXacc M2(cols,rows);
    MatrixXacc M3(rows,rows);

    for(unsigned i=0; i<(num_streams+1); i++)
        for(unsigned j=0; j<num_items; j++)
            if(i==num_streams)
                M1(i,j) = 1;
            else
                M1(i,j) = j;
        
    M2 = M1.transpose();

    std::vector<float> durations_collect;

    for(unsigned r=0; r<repetitions; r++) {
        // Threaded Amnes sums Collection
        //  - split as evenly as possible at 32-byte boundaries
        auto const t0 = std::chrono::system_clock::now();
        {
           M3 = M1*M1.transpose();
	   // M3 = M1* M2;
        }
        
        // Sum all partial sums
        auto const t1 = std::chrono::system_clock::now();
        
        float const d0 = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
       
        durations_collect.push_back(d0); 

    }

    //std::cout<<"M3(0,0): "<<M3(0,0)<<std::endl;
    //std::cout<<M3<<std::endl;
    //Get stats
    //collect-throughtput
    std::vector<float> th_collect;
    std::vector<float> compute_duration;

    for(unsigned i=0; i<1; i++) {
        for(unsigned r=0; r<repetitions; r++) {
            if(i==0){
                th_collect.push_back((sizeof(data_t)*num_items*num_streams)/durations_collect[r]);
                compute_duration = durations_collect;
            }
        }

        std::sort(th_collect.begin(),th_collect.end());
        std::sort(compute_duration.begin(),compute_duration.end());
    
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
            std::string name = "ss_tput_cllct_"+std::to_string(num_streams)+"_"+std::to_string(databits)+".txt";
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
    
    return  0;
}
