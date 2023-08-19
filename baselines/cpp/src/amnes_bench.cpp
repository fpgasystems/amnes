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

#include "amnes.hpp"

#include <stdio.h>
#include <stdlib.h>

// #include <boost/align/is_aligned.hpp>
// bool is_aligned(const void* ptr, std::size_t alignment) noexcept;

int main(int argc, char* argv[]) {

    // Validate and Capture Arguments
    if (argc < 6) {
        std::cout << "Usage: " << argv[0] << " <num_items> <num_streams> <databits> <num_threads> <repetitions> <filename> \n";
        return  1;
    }

    unsigned const per_block = M_STREAMS;

    size_t const num_items = strtoul(argv[1], nullptr, 0);
    if(num_items%per_block != 0) {
        std::cerr << "Num_items must be a multiple of " << per_block << std::endl;
        return  1;
    }

    unsigned const num_streams = strtoul(argv[2], nullptr, 0);
    if(num_streams < 2) {
        std::cerr << "Correlation needs minimum 2 streams." << std::endl;
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

    // Collectors for Sum of Elements, Sum of Squares, Sum of Products per thread
    std::vector<AmnesCollector<data_t, acc_t>> collectors;
    for(unsigned i = 0; i < num_threads; i++)  collectors.emplace_back(num_streams);
    
    AmnesCollector<data_t, acc_t> overall_collector(num_streams);  

    // Allocate & Populate Input Memory
    data_t* input = static_cast<data_t*>(aligned_alloc(64, sizeof(data_t)*num_items*num_streams));
    //std::cout<<boost::alignment::is_aligned(&input[0], CACHE_LINE_SIZE)<<std::endl;//is_aligned(&input)<<std::endl;
	
    if(filename.empty()) {
        data_t v    = 0;
        
        for(unsigned i=0; i < num_items; i++) {
            for(unsigned j=0; j<num_streams; j++)
                 input[i*num_streams + j] = v;

            v++;
        }
    }
    else {
        char content[1024];
        unsigned i = 0;
        unsigned j = 0;
        
        while(!source_file.eof() && i<num_items){

            char *v = strtok(content, ",");
            j = 0;

            while (v){

                if(i>0 and j>0){
                   input[num_streams*(i-1) + (j-1)] = (data_t)atoi(v); 
                   // std::cout<<"i,j,v:"<<i-1<<","<<j-1<<","<<values[i-1][j-1]<<std::endl;             
                }

                v =strtok(NULL, ",");
                j++;
            }
            i++;          
        }
        source_file.close();
    }
    
    std::vector<float> durations_collect;
    std::vector<float> durations_merge;
    std::vector<float> durations_total;
 
    std::vector<float> pcc_coefficients(num_streams*(num_streams-1)>>1);

    //std::cout<<"Num blocks:"<<num_blocks<<std::endl;
    //std::cout<<"Per blocks:"<<per_block<<std::endl;
    
    for(unsigned r=0; r<repetitions; r++) {
        // Threaded Amnes sums Collection
        //  - split as evenly as possible at 32-byte boundaries
        auto const t0 = std::chrono::system_clock::now();
        {
            std::vector<std::thread> threads;

            size_t ofs = 0;
            for(unsigned i = 0; i<num_threads; i++) {
                size_t const  nofs  = (((i+1)*num_blocks)/num_threads) * per_block;

                size_t const  cnt = nofs-ofs;
                //std::cout<<"Oft: "<<ofs<<std::endl;
                //std::cout<<"Nofs: "<<nofs<<std::endl;
                //std::cout<<"Cnt: "<<cnt<<std::endl;
                //std::cout<<"Real ofst:"<<ofs*num_streams<<std::endl;
                AmnesCollector<data_t, acc_t>& clct(collectors[i]);
                threads.emplace_back([data=&input[ofs*num_streams], cnt, &clct](){ clct.collect(data, cnt); });
                cpu_set_t cpus;
                CPU_ZERO(&cpus);
                CPU_SET((i<<1)%num_cores, &cpus);
                pthread_setaffinity_np(threads.back().native_handle(), sizeof(cpus), &cpus);
                ofs = nofs;
            }   
            // Wait for all to finish
            for(std::thread& t : threads) t.join();
        }
        
        // Sum all partial sums
        auto const t1 = std::chrono::system_clock::now();
        for(unsigned  i = 0; i < num_threads; i++) {
            overall_collector.merge(collectors[i]);
        }
        
        auto const t2 = std::chrono::system_clock::now();

        overall_collector.get_pcc_coefficients(num_items, pcc_coefficients);

        auto const t3 = std::chrono::system_clock::now();

        float const d0 = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        float const d1 = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t0).count();
        float const d2 = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t0).count();
        
        durations_collect.push_back(d0); 
        durations_merge.push_back(d1);
        durations_total.push_back(d2);

        for(unsigned  i = 0; i < num_threads; i++) collectors[i].clean();

        overall_collector.clean(); 
    }     
    free(input);
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
            else
                if(i==1){
                    th_collect.push_back((sizeof(data_t)*num_items*num_streams)/durations_merge[r]);    
                    compute_duration = durations_merge;
                }
                else{
                    th_collect.push_back((sizeof(data_t)*num_items*num_streams)/durations_total[r]);
                    compute_duration = durations_total;
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
            std::string name = "amnes_tput_cllct_"+std::to_string(num_streams)+"_"+std::to_string(databits)+".txt";
            file.open (name, std::ios::app);         
        }
        else 
            if(i==1){
                std::string name = "amnes_tput_merge_"+std::to_string(num_streams)+"_"+std::to_string(databits)+".txt";
                file.open (name, std::ios::app);
            }
            else{
            std::string name = "amnes_tput_total_"+std::to_string(num_streams)+"_"+std::to_string(databits)+".txt";
            file.open (name, std::ios::app);
            }

        file.seekg (0, std::ios::end);
        int length = file.tellg();

        if(length == 0)
            file<<"items num_streams databits threads repet min max p1 p5 p25 p50 p75 p95 p99 iqr liqr uiqr dmin dmax pcc"<<std::endl; 
    
        file<< num_items <<" "<< num_streams<<" "<<databits<<" "<<num_threads <<" "
            << repetitions <<" "<< std::setprecision(5) 
            << min <<" "<< max <<" "
            << p1 <<" "<< p5 <<" "<< p25 <<" "
            << p50 <<" "<< p75 <<" "<< p95 <<" "<<p99<<" "
            << iqr <<" "<< lower_iqr <<" "
            << upper_iqr<<" "
            << compute_duration[0]<<" "
            << compute_duration[repetitions-1]<<" "
            << pcc_coefficients[0]<<std::endl;
    
        file.close();
    }
    
    return  0;
}
