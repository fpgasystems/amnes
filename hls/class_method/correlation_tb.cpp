#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <algorithm>
#include <map>

#include "correlation_parameters.hpp"
#include "correlation.hpp"

unsigned const ITEMS    = 128;//131072;//128;
unsigned const STRM_SRC  = 0; // 0 - locally generated, 1 - read from CSV file
unsigned const STRM_TYPE = 0; // 0 - integer input type, 1 - float input type 

result_data_t SUM[N];
result_data_t SUMSQUARES[N];
result_data_t SUMPRODS[P];

template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs) {
    // No predicate needed because there is operator== for pairs already.
    
    return (lhs.size() == rhs.size() && 
            std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

result_data_t calculateSum(data_t *x, unsigned noSamples){
   data_t *ptrX = x;
   
   result_data_t sum = 0;
  
   while (noSamples!=0){
        sum += *ptrX;  
        ptrX+=N;
        
        noSamples--;
   }

   return sum;
}

result_data_t calculateSumOfProducts(data_t *x, data_t *y, unsigned noSamples){
   data_t *ptrX = x;
   data_t *ptrY = y;
   
   result_data_t sum = 0;
  
   while (noSamples!=0){
        sum += (*ptrX) * (*ptrY);

        ptrX+=N; ptrY+=N;
       
        noSamples--;
   }
  
   return sum;
}

int inputStream(hls::stream<input_t>      & dataStreams,                  
                 std::map<unsigned, float> & correlationMap){
    
    data_t values[ITEMS][N];
    input_t dataBits; 
   
    // unsigned noResults = 2*STREAMS + STREAMS*(STREAMS-1)/2;
    
    // Read the file
    if(STRM_SRC==1){
        // values read from a CSV file
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current working dir: %s\n", cwd);
            ///home/monica_chiosa/projects/correlation/hls/build/correlation_prj/solution1/csim/build
        } 
        
        std::string filename = "../../../../../../benchmarks/python/test/test_int_16_"+std::to_string(ITEMS)+".csv";
        std::cout<<"Filename: "<< filename <<std::endl;

        FILE *file = fopen((const char*)filename.c_str(), "r");
        if(!file){
            std::cout<<"Could not open the file"<<std::endl;
            return 1;
        }
        
        char content[1024];
        int i = 0;
        int j = 0;
        
        while(fgets(content,1024,file)){

            char *v = strtok(content, ",");
            j = 0;

            while (v){

                if(i>0 and j>0){
                  
                   values[i-1][j-1] = atoi(v); 
                   // std::cout<<"i,j,v:"<<i-1<<","<<j-1<<","<<values[i-1][j-1]<<std::endl;             
                }

                v =strtok(NULL, ",");
                j++;
            }
            i++;          
        }
        fclose(file);
    }
    else{
        // values locally generated
        for(unsigned i = 0; i < ITEMS; i++) 
            for(unsigned j = 0; j < N; j++) 
                values[i][j] = i;//100+(j+1)*(int)pow(-1,i);
    }

    // Generate Input Stream
    for(unsigned i = 0; i < ITEMS; i++) {
        input_t  x;
        x.last = (i+1==ITEMS);
        
        for(unsigned j = 0; j < N; j++) {
            x.val((j+1)*D-1, j*D) = values[i][j];
            //std::cout<<values[i][j]<<" ";
        }
        //std::cout<<std::endl;

        dataStreams.write(x);
    }

    // Generate Input Stream
    //for(unsigned i = 0; i < N; i++) {
    //    std::cout<<"stream: "<<i<<" values:"<<std::endl;
    //    
    //    for(unsigned j = 0; j < ITEMS; j++) {
    //        std::cout<<values[j][i]<< " ";
    //    }
    //    std::cout<<std::endl;
    //}

    for(unsigned i=0; i<N; i++){
        // Calculate the sum for each stream
        data_t *ptrValue = &values[0][i];
        SUM[i] = calculateSum(ptrValue, ITEMS);

        // Calculate the sum of squares for each stream
        SUMSQUARES[i] = calculateSumOfProducts(ptrValue, ptrValue, ITEMS);
        
        //std::cout<<"Sum of elements: "<< i<<" value: "<< SUM[i] << std::endl; 
        //std::cout<<"Sum of squares : "<< i<<" value: "<< SUMSQUARES[i] << std::endl;     
    }
    
    for (unsigned i=0; (i+1)<N; i++){
        for (unsigned j=i+1; j<N; j++){

            data_t *ptrValue1 = &values[0][i];
            data_t *ptrValue2 = &values[0][j];

            unsigned index = i*N-(i*(i+1))/2+j-i-1;
          //*  std::cout<<"i,j: "<<i<<","<<j<<" index:"<<index<<std::endl;
            SUMPRODS[index] = calculateSumOfProducts(ptrValue1, ptrValue2, ITEMS);
          //*  std::cout<<"Sum of products : "<< index<<" value: "<< SUMPRODS[index] << std::endl;
            
            result_data_t const sum_e1  = SUM[i];
            result_data_t const sum_e2  = SUM[j];

            result_data_t const sum_sq1 = SUMSQUARES[i];
            result_data_t const sum_sq2 = SUMSQUARES[j];

            result_data_t const numerator   = ITEMS*SUMPRODS[index] - sum_e1*sum_e2;
            result_data_t const denominator = (ITEMS*sum_sq1 - sum_e1*sum_e1) * (ITEMS*sum_sq2-sum_e2*sum_e2);
            
            float value = 0.0f;

            if(denominator!=0)
                value = (float)numerator/pow(denominator,0.5);   

            //if(isnan(value))
            //    value = 0.0f;

            correlationMap[index] = value;     
      }                
    } 

    return 0; 
}

int main() {
    hls::stream<input_t>  src("input_stream");
    hls::stream<coeff_t>  dst("results_stream");

    std::map<unsigned, float> expectedResult;
    std::map<unsigned, float> result;

    if(inputStream(src, expectedResult)!=0){
        return 1;
    }
    
    std::cout<<"Value in the test bench written! "<<std::endl;
    std::cout<<"-----"<<std::endl;

    // Process and Check Output
    bool correct = true;
    bool done = false;
    unsigned products = 0;
    unsigned counter = 0;
    unsigned val_outside = 0;

    while ((!done) & correct) {
        correlation_top(src, dst);

        coeff_t  p;

        if(dst.read_nb(p)){

            result[counter] = p.val;
            //* std::cout<<std::hex<<"Value: "<<result[counter] << std::endl;

            if(expectedResult[counter]!=p.val){
                std::cout<<std::hex<<"Expected Value: "<<expectedResult[counter] << std::endl;
                std::cout<<std::hex<<"Output Value: "<<p.val <<" last:"<<p.last<< std::endl;
            }

            if(p.val<-1 or p.val>1){
                //std::cout<<std::hex<<"Expected Value: "<<expectedResult[counter] << std::endl;
                //std::cout<<std::hex<<"Output Value: "<<p.val <<" last:"<<p.last<< std::endl;
                val_outside++;
            }

            counter++;
        }

        if(p.last){
	    
	    if (D==32)
        	correct = map_compare(expectedResult,result);
            else
        	correct = 1;

            if (!correct)
                std::cerr << "[ERROR] Result is not matching expected result" << std::endl;
            
            done = p.last;
        }

    }

    std::cout<<std::dec<<"Received results           : "<<counter<<std::endl;
    std::cout<<std::dec<<"Coefficients outside [-1,1]: "<<val_outside<<std::endl;

    return correct? 0 : 1;
    

} // main()
