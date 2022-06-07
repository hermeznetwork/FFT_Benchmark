#include <benchmark/benchmark.h>
#include "goldilocks/goldilocks.hpp"
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <math.h>
#include <random>
#include <iostream>



#define SIZE 8388608

int main(int argc, char **argv ){


    uint64_t columns   = atoi(argv[1]);
    uint64_t blocks    = atoi(argv[2]);
    int reps           = atoi(argv[3]);
    uint64_t nThreads  = atoi(argv[4]);
    uint64_t colsBlock = columns/blocks;

    omp_set_num_threads(nThreads);
    printf("Arguments: columns=%lu, blocks=%lu,reps=%d, nThreads=%lu, colsBlock=%lu\n",columns, blocks, reps, nThreads,colsBlock);

    #pragma omp parallel
    {
	#pragma omp master
	{
           printf("Using %d threads\n",omp_get_num_threads());    
	}

    }

    uint64_t length = SIZE;
    printf("Columns per proc=%lu(-1) reps=%d\n",columns,reps);
    Goldilocks g(length, 2);
    
    uint64_t *pol_ext_1 = (uint64_t *)malloc(length * sizeof(uint64_t) *columns);
    uint64_t *pol_ext_2 = (uint64_t *)malloc(length * sizeof(uint64_t) *columns);
    uint64_t *aux1 = (uint64_t *)malloc(length * sizeof(uint64_t) * columns);
    uint64_t *aux2 = (uint64_t *)malloc(length * sizeof(uint64_t) *columns);

    
    // Fibonacci

    // INITIALIZATION NON-VECTORIZED
    #pragma omp parallel for
    for(uint64_t k=0; k< columns; k++){ 
	uint64_t offset = k*length;;
	pol_ext_1[offset] = 1+k;
	pol_ext_1[offset+1] = 1+k;
	for (uint64_t i = 2; i < length; i++)
	{
	        pol_ext_1[offset+i] = g.gl_add(pol_ext_1[offset+i-1], pol_ext_1[offset+i-2]);
	}
    }

    // INITIALIZATION VECTORIZED
    for(uint64_t k=0; k<blocks; k++){
       uint64_t offset0 = colsBlock*length*k;	    
 #pragma omp parallel for
       for(uint64_t j=0; j<length; ++j){
	  uint64_t offset1 = offset0+j*colsBlock;
	  for (uint64_t i = 0; i < colsBlock; i++)
	  {
	     uint64_t offset2 = (i+k*colsBlock)*length;
	     pol_ext_2[offset1+i]=(pol_ext_1[j+offset2]);
	 }
       }
    }

    //
    // NON-VECTORIZED EXECUTION
    //
    double st = omp_get_wtime();  
    for (int k=0; k<reps; ++k)
    {
#pragma omp parallel for    
        for (u_int64_t i = 0; i < columns; i++)
        {
            int offset = i*length;
            g.ntt(pol_ext_1+offset, length,aux1+offset);
        }
    }
    double runtime1 = omp_get_wtime() - st;
    printf("NON-VECTORIZED// Columns: %lu, time:%f\n", columns, runtime1 / reps);
   
    //
    // VECTORIZED EXECUTION
    //
    st = omp_get_wtime();  
    for (int k=0; k<reps; ++k)
    {
	    
//#pragma omp parallel for 
//        for (u_int64_t i = 0; i < blocks; i++){
//       
//         
           #pragma omp task 
	   {
	      int i = 0;
	      int offset = i*(columns/blocks)*length;
              g.ntt_block(&pol_ext_2[offset], length, colsBlock,aux2);
	   }
          
 	   #pragma omp task
	   {
	      int i = 1;
	      int offset = i*(columns/blocks)*length;
              g.ntt_block(&pol_ext_2[offset], length, colsBlock,aux2);

	   }
           #pragma omp taskwait
//	}
    }
    double runtime2 = omp_get_wtime() - st;
    printf("VECTORIZED    // Columns: %lu, time:%f\n", columns, runtime2 / reps);
    

    //
    // CHECK RESULTS
    //
    for(uint64_t k=0; k<blocks; k++){
       uint64_t offset0 = colsBlock*length*k;	    
 #pragma omp parallel for
       for(uint64_t j=0; j<length; ++j){
	  uint64_t offset1 = offset0+j*colsBlock;
	  for (uint64_t i = 0; i < colsBlock; i++)
	  {
	     uint64_t offset2 = (i+k*colsBlock)*length;
	     pol_ext_2[offset1+i]=(pol_ext_1[j+offset2]);
	 }
       }
    }

    u_int64_t cont = reps*columns;
    printf("Ffts done: %lu, length: %lu speedup: %f\n", cont,length,runtime1/runtime2);
    
    free(pol_ext_1);
    free(pol_ext_2);
    free(aux1);
    free(aux2);

    return 0;
}

