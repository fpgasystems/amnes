#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ap_int.h>

#include <map>
#include <random>
#include <chrono>

#include "cm_operations.hpp"
#include "../globals.hpp"

//static unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
ap_uint<RESW> SUM[STREAMS];
ap_uint<RESW> SUMSQUARES[STREAMS];
ap_uint<RESW> SUMPRODS[STREAMS*(STREAMS-1)/2];

template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs) 
{
    // No predicate needed because there is operator== for pairs already.
    
    return (lhs.size() == rhs.size() && 
            std::equal(lhs.begin(), lhs.end(), rhs.begin(),rhs.end()));
}

uint64_t calculateSum(uint32_t *x, uint32_t noSamples)
{
   uint32_t *ptrX = x;
   
   uint64_t sum = 0;
  
   while (noSamples!=0)
   {
        sum += *ptrX;  
        ptrX+=STREAMS;
        
        noSamples--;
   }

   return sum;
}

uint64_t calculateSumOfProducts(uint32_t *x, uint32_t *y, uint32_t noSamples)
{
   uint32_t *ptrX = x;
   uint32_t *ptrY = y;
   
   uint64_t sum = 0;
  
   while (noSamples!=0)
   {
        sum += (*ptrX) * (*ptrY);

        ptrX+=STREAMS; ptrY+=STREAMS;
       
        noSamples--;
   }
  
   return sum;
}


void inputStream(hls::stream<net_axis<LINEW> >  &         dataStreams,                 
                 ap_uint<REGW>                            noSamples,
                 ap_uint<REGW>                            noStreams, 
                 std::map<ap_uint<RESW>, ap_uint<MEMW> >& correlationMap)
{
    //std::default_random_engine rand_gen(seed);
    //std::uniform_int_distribution<int> valueDistr(0, 100); //std::numeric_limits<int>::max());

   uint32_t values[SAMPLES][STREAMS];
   net_axis<LINEW> dataBits; 
   
   unsigned noResults = 2*STREAMS + STREAMS*(STREAMS-1)/2;

   for (uint32_t i=0; i < (uint32_t)(noSamples); i++)
   {    
        uint32_t *ptrValue = values[i];

        for (uint32_t j=0; j < (uint32_t)(noStreams); j++)
        {   
          /*  uint32_t streamValue; 
            switch(i)
            {
               case 0 : 
                   {
                       streamValue = j;                        
                   }
                   break;
               case 1 :
                   {
                        streamValue = 3*j+10;                       
                   }
                   break;
            }
          */

            *ptrValue = 1; //streamValue;
            ptrValue++;
            dataBits.data((j+1)*DATAW-1, j*DATAW) = 1; //streamValue;
            dataBits.keep((j+1)*DATAW/8-1,j*DATAW/8) = 0xF;
            dataBits.last = 0x0;
        }  

         dataStreams.write(dataBits);    
         std::cout<<"Sample: "<< i<<" Data: "<<std::hex<< dataBits.data << std::endl; 
    }

    for(uint32_t i=0; i<(uint32_t)(noStreams); i++)
    {
      // Calculate the sum for each stream
      uint32_t *ptrValue = &values[0][i];
      SUM[i] = (ap_uint<RESW>)(calculateSum(ptrValue, (uint32_t)noSamples));

     // std::cout<<"Sum of elements: "<< i<<" value: "<<std::hex<< SUM[i] << std::endl;
       
      // Calculate the sum of squares for each stream
      SUMSQUARES[i] = (ap_uint<RESW>)(calculateSumOfProducts(ptrValue, ptrValue, (uint32_t)noSamples));
      
     // std::cout<<"Sum of squares : "<< i<<" value: "<<std::hex<< SUMSQUARES[i] << std::endl;     
    }
    
    for (uint32_t i=0; i<((uint32_t)(noStreams)-1); i++)
    {
      for (uint32_t j=i+1; j<(uint32_t)noStreams; j++)
      {
        uint32_t *ptrValue1 = &values[0][i];
        uint32_t *ptrValue2 = &values[0][j];

        uint32_t index = i*16-(i*(i+1))/2+j-i-1;
        //std::cout<<"i,j: "<<i<<","<<j<<" index:"<<index<<std::endl;
        SUMPRODS[index] = (ap_uint<RESW>)(calculateSumOfProducts(ptrValue1, ptrValue2, (uint32_t)noSamples));
      }
                       
    }
      
    ap_uint<MEMW> sumConcatenation = 0;
    unsigned resultsOnOneLine = MEMW/RESW;

      for (uint32_t i=0; i < (uint32_t)(noResults/resultsOnOneLine); i++)
      {

        if(STREAMS <= resultsOnOneLine)
        {
          if(i==0)
           for(unsigned j=0; j<STREAMS; j++)
           {
              sumConcatenation((j+1)*RESW-1,j*RESW) = SUM[j];
           }
          else
            if(i==1)
              for(unsigned j=0; j<STREAMS; j++)
              {
                 sumConcatenation((j+1)*RESW-1,j*RESW) = SUMSQUARES[j];
              }
            else
             {
                for(unsigned j=0; j<STREAMS; j++)
                {
                   sumConcatenation((j+1)*RESW-1,j*RESW) = SUMPRODS[i*STREAMS+j];
                }
             }
        }

        correlationMap[i] = sumConcatenation;
      }

      if(noResults%resultsOnOneLine !=0)
      {
        for(unsigned j=0; j<(noResults%resultsOnOneLine); j++)
        {
           sumConcatenation((j+1)*RESW-1,j*RESW) = SUMPRODS[((uint32_t)(noResults/resultsOnOneLine))*STREAMS+j];
        }

        correlationMap[(unsigned)(noResults/resultsOnOneLine)] = sumConcatenation;
      } 
}


int main(int argc, char* argv[])
{
    hls::stream<net_axis<LINEW> > s_axis_input_attribute;

   // hls::stream<memCmd>          m_axis_write_cmd("m_axis_write_cmd");               
    hls::stream<net_axis<MEMW> >  m_axis_write_data("m_axis_write_data"); 

    ap_uint<REGW> numSamples = ap_uint<REGW>(SAMPLES);
    ap_uint<REGW> numStreams = ap_uint<REGW>(STREAMS);
    
    ap_uint<REGW> baseAddr   = 0x00000000;
                                       
    std::map<ap_uint<RESW>, ap_uint<MEMW>> expectedResult;

    inputStream(s_axis_input_attribute, numSamples, numStreams, expectedResult);
    //std::cout<<std::hex<<"expected:"<<expectedResult[0]<<std::endl;
    
    int count = 0;

    while(count < (numSamples + 10000))
    {
       correlation( s_axis_input_attribute,            
                    m_axis_write_data,
                    numSamples,
                    baseAddr);

       count++;
    }
 
    bool correct = true;

    uint64_t expectedAddress = baseAddr;
    uint64_t totalLength = 0;
    uint64_t expectedLength = 0;

    // retrive memory command
    // while(!m_axis_write_data.empty())
    // {
    //     memCmd currMemCmd = m_axis_write_cmd.read();
    //     
    //     //Chech length and address
    //     if (currMemCmd.addr != expectedAddress)
    //     {
    //         std::cerr << "[ERROR] DMA address not correct; expected: " << expectedAddress << ", received: " << currMemCmd.addr << std::endl;
    //         correct = false;
    //     }
    // 
    //     expectedAddress += currMemCmd.len;
    //     totalLength += currMemCmd.len;
    // }

    // retrive memory data
    std::map<ap_uint<RESW>, ap_uint<MEMW>> result;
    int numberSums = 0;

    while (!m_axis_write_data.empty())
    {
        net_axis<MEMW> word = m_axis_write_data.read(); 
        result[numberSums] = word.data;
        std::cout<<std::dec<<"Sum "<<numberSums<<": "<<std::hex<<word.data<<std::endl;
        numberSums++;
    }
    std::cout<<"Correlation " << numberSums << std::endl;
//
//        double pcc;
//        ap_uint<RESW> numerator = SAMPLES*word.data(319,256) - word.data(63,0)*word.data(127,64);
//        double denominator1 = (double)(SAMPLES*word.data(191,128)-word.data(63,0)*word.data(63,0));
//        double denominator2 = (double)(SAMPLES*word.data(255,192)-word.data(127,64)*word.data(127,64));
//        
//        pcc = numerator/(std::sqrt(denominator1) * std::sqrt(denominator2));
//        std::cout<<"  Result    : "<<pcc<<std::endl;
//        numberSums++; 
//        
//    }
//     
//    //Check result by comparing maps
//     if (correct)
//     {
//         correct = map_compare(expectedResult, result);
//         if (!correct)
//             std::cerr << "[ERROR] Result is not matching expected result" << std::endl;
//     }
//
//    //if not correct print the two maps
//    if (!correct)
//    {
//        if (expectedResult.size() != result.size())
//            std::cerr << "[ERROR] size mismatch, expected: " << expectedResult.size() << ", received: " << result.size() << std::endl;
//
//        std::map<ap_uint<RESW>, ap_uint<MEMW>>::const_iterator it;
//        std::cout << "------ [RESULT] ------" << std::endl;
//        std::cout << "expected\tactual" << std::endl;
//        for (it = expectedResult.begin(); it != expectedResult.end(); it++)
//        {
//
//            if (result.find(it->first) != result.end())
//            {
//                if( it->second !=result[it->first] )
//                {
//                  std::cout << std::dec << "<" << it->first << "," << std::hex<<it->second << ">\n";                 
//                  std::cout << std::dec <<"<" << it->first << "," << std::hex<<result[it->first] << ">"<<std::endl;
//
//                }
//            }
//            else
//            {
//                std::cout << std::dec << "<" << it->first << "," << it->second << ">\t";
//                std::cout << "missing"<<std::endl;
//            }
//
//        }
//        std::cout << "---------------------" << std::endl;
//    }
//
//   return (correct ? 0 : -1);*/
}
