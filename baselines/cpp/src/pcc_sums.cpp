
// source https://www.geeksforgeeks.org/program-find-correlation-coefficient/
// Program to find correlation coefficient 
#include<bits/stdc++.h> 
#include <boost/program_options.hpp>
#include <chrono>
#include <thread>
#include <random>
  
using namespace std; 
static unsigned seed = 12; //std::chrono::system_clock::now().time_since_epoch().count();

void fillRandomData(uint32_t* matrix, uint32_t n, uint32_t m) {
    std::default_random_engine rand_gen(seed);
    std::uniform_int_distribution<int> distr(0, 100);//std::numeric_limits<std::uint32_t>::max());
  
    uint32_t* uPtrM = matrix;

    for (uint32_t i=0; i<m; i++) {
        for (uint32_t j=0; j<n; j++)
            *(uPtrM+j) = distr(rand_gen);
        
        uPtrM+=n;
    }   
}
  
// function that returns correlation coefficient. 
void correlationCoefficient(uint32_t* matrix, uint32_t n, uint32_t m, float* results) 
{ 
    uint32_t* ptrM = matrix;
    float* ptrR = results;

    int64_t sum[m], product[m*(m-1)/2]; 
    int64_t square[m]; 

    float corr;
    int32_t result_counter = 0;
    
    for(uint32_t j=0; j<m; j++){
        for (uint32_t i=0; i<n; i++){ 
            // sum of elements of each stream.
            uint32_t value = *(ptrM+n*j+i);
            sum[j]    += value; 
            // sum of squares of elements of each stream.
            square[j] += value * value;
        }
    }

    for(uint32_t j=0; j<(m-1); j++){
        for(uint32_t k=(j+1); k<m; k++){
            for(uint32_t i=0; i<n; i++)
                product[result_counter] = *(ptrM+n*j+i) * (*(ptrM+n*k+i));

            result_counter++;
        }
    } 
    
    // use formula for calculating correlation coefficient. 
    result_counter = 0;
    for(uint32_t j=0; j<(m-1); j++){
        corr = 0.0;
        for(uint32_t k=(j+1); k<m; k++){
            int64_t numerator = n * product[result_counter] - sum[j] * sum[k];
            int64_t denominator = (n * square[j] - sum[j] * sum[j]) * (n * square[k] - sum[k] * sum[k]);
             
            if (denominator!=0)
                corr = (float)(numerator)/ sqrt(denominator); 

            *(ptrR + result_counter) = corr;
            result_counter++;
        }  
    }
} 
  
// Driver function 
int main(int argc, char *argv[]) {
     
    boost::program_options::options_description programDescription("Allowed options");
    programDescription.add_options()("elements,n", boost::program_options::value<uint32_t>(), "Length of the streams.")
                                    ("repetitions,r", boost::program_options::value<uint32_t>(), "Number of repetitions.")
                                    ("streams,s", boost::program_options::value<uint32_t>(), "Number of streams.");

    boost::program_options::variables_map commandLineArgs;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, programDescription), commandLineArgs);
    boost::program_options::notify(commandLineArgs);
  
    uint32_t numberElements    = 100;
    uint32_t numberRepetitions = 100;
    uint32_t numberStreams     = 2;

    if (commandLineArgs.count("elements") > 0) {
        numberElements = commandLineArgs["elements"].as<uint32_t>();
    } 

    if (commandLineArgs.count("repetitions") > 0) {
        numberRepetitions = commandLineArgs["repetitions"].as<uint32_t>();
        cout<<numberRepetitions<<endl;
    }

    if (commandLineArgs.count("streams") > 0) {
        numberStreams = commandLineArgs["streams"].as<uint32_t>();
        if(numberStreams<2){
            cout<<"Stream number should be bigger or equal to 2."<<endl;
            numberStreams = 2;
        }
    }
    
    cout<<"Repetitions  :"<<numberRepetitions<<endl;
    cout<<"Streams      :"<<numberStreams<<endl;
    cout<<"Values/Stream:"<<numberElements<<endl;


    //Allocate memory for streams and results
    uint32_t* bufferData = (uint32_t*)malloc(numberElements*numberStreams*sizeof(uint32_t));
    float*    bufferResults = (float*)malloc((numberStreams*(numberStreams-1)/2)*sizeof(float));
    fillRandomData(bufferData, numberElements, numberStreams);

    //Function call to correlationCoefficient. 
    
    double totalDurationUs = 0.0;
    std::vector<double> durations;

    for (uint32_t r=0; r<(numberRepetitions+1); r++){
        if(r==0)
            continue;
        
        auto start = std::chrono::high_resolution_clock::now();
        correlationCoefficient(bufferData, numberElements, numberStreams, bufferResults);
        auto end = std::chrono::high_resolution_clock::now();
        double durationUs = (std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count() / 1000.0);
        if (r != 0) {
            durations.push_back(durationUs);
        }
    } 

    //Get stats
    for (uint32_t r = 0; r<numberRepetitions; r++) {
        totalDurationUs += durations[r];
         //std::cout << "duration[" << r << "]: " << durations[r] << std::endl;
    }

    double avgDurationUs = totalDurationUs /(double)numberRepetitions;
    double stddev = 0.0;
    for (uint32_t r = 0; r<numberRepetitions; r++) {
         stddev += ((durations[r] - avgDurationUs) * (durations[r] - avgDurationUs));
    }
    stddev /= numberRepetitions;
    stddev  = sqrt(stddev);
    
    std::sort(durations.begin(), durations.end());
    double min = durations[0];
    double max = durations[numberRepetitions-1];
    double p25 = durations[(numberRepetitions/4)-1];
    double p50 = durations[(numberRepetitions/2)-1];
    double p75 = durations[((numberRepetitions*3)/4)-1];
    double p1 = 0.0;
    double p5 = 0.0;
    double p95 = 0.0;
    double p99 = 0.0;
    double iqr = p75 - p25;
    std::cout << "iqr: " << iqr << std::endl;
    double lower_iqr = p25 - (1.5 * iqr);
    double upper_iqr = p75 + (1.5 * iqr);
    if (numberRepetitions >= 100) {
        p1  = durations[((numberRepetitions)/100)-1];
        p5  = durations[((numberRepetitions*5)/100)-1];
        p95 = durations[((numberRepetitions*95)/100)-1];
        p99 = durations[((numberRepetitions*99)/100)-1];
    }
    
    double pliqr = durations[0];
    double puiqr = durations[0];
    for (uint32_t r = 0; r < numberRepetitions; ++r) {
        pliqr = durations[r];
        if (pliqr > lower_iqr) {
            break;
        }
    }
    for (uint32_t r = 0; r < numberRepetitions; ++r) {
        if (durations[r] > upper_iqr) {
            break;
        }
        puiqr = durations[r];
    }

    std::cout << std::fixed << "Duration[us]: " << avgDurationUs << std::endl;
    std::cout << std::fixed << "Min: " << min << std::endl;
    std::cout << std::fixed << "Max: " << max << std::endl;
    std::cout << std::fixed << "Median: " << p50 << std::endl;
    std::cout << std::fixed << "1th: " << p1 << std::endl;
    std::cout << std::fixed << "5th: " << p5 << std::endl;
    std::cout << std::fixed << "25th: " << p25 << std::endl;
    std::cout << std::fixed << "75th: " << p75 << std::endl;
    std::cout << std::fixed << "95th: " << p95 << std::endl;
    std::cout << std::fixed << "99th: " << p99 << std::endl;
    std::cout << std::fixed << "Lower IQR: " << pliqr << std::endl;
    std::cout << std::fixed << "Upper IQR: " << puiqr << std::endl;
//    std::cout << std::fixed << "Duration stddev: " << stddev << std::endl;
     
    free(bufferData);
    free(bufferResults);

    return 0; 
} 
