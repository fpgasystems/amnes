#ifndef CORRELATION_HPP
#define CORRELATION_HPP

#include <map>

unsigned const L = 512;       // Line width  
unsigned const D = 32;        // Data size
unsigned const R = 64;        // Result size
unsigned const N = L/D;       // Number of input lanes
unsigned const P = N*(N-1)/2; // Number of products

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

#endif