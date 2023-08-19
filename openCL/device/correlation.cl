// Copyright (c) 2020, Systems Group, ETH Zurich
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#define UNROLL_FACTOR 1

__kernel
void correlation(__global const float* restrict x, __global const float* restrict y, __global float* restrict se_x, __global float* restrict se_y, __global float* restrict ss_x, __global float* restrict ss_y, __global float* restrict sp, int N) {

 float sum_x = 0.0f;
 float sum_y = 0.0f;

 float sum_sx = 0.0f;
 float sum_sy = 0.0f;

 float sum_p = 0.0f;
 
   for (int i=0; i<(N/UNROLL_FACTOR); i++)
   {
    float temp_x = 0.0f;
    float temp_y = 0.0f;

    float temp_sx = 0.0f;
    float temp_sy = 0.0f;
    
    float temp_p = 0.0f;
 
    #pragma unroll UNROLL_FACTOR
    for (int j=0; j<UNROLL_FACTOR; j++)
    {
      // Prefetch value from the main memory => FIFO instances
      float value_x = x[i*UNROLL_FACTOR+j];
      float value_y = y[i*UNROLL_FACTOR+j];
     
      temp_x += value_x;   
      temp_y += value_y;
   
      temp_sx += value_x*value_x;
      temp_sy += value_y*value_y;

      temp_p += value_x*value_y;
    }

    sum_x += temp_x; 
    sum_y += temp_y;
    
    sum_sx += temp_sx;
    sum_sy += temp_sy;
    
    sum_p += temp_p;
  } 

 *se_x = sum_x;
 *se_y = sum_y;
 
 *ss_x = sum_sx;
 *ss_y = sum_sy;

 *sp   = sum_p;
}
