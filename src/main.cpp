#include <benchmark/benchmark.h>
#include "goldilocks/goldilocks.hpp"
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <math.h>
#include <random>
#include <iostream>

#ifdef LIKWID_PERFMON
#include <likwid-marker.h>
#endif


#define SIZE 1<<4 

int main(int argc, char **argv ){


    uint64_t columns   = atoi(argv[1]);
    uint64_t blocks    = atoi(argv[2]);
    int reps           = atoi(argv[3]);
    uint64_t nThreads  = atoi(argv[4]);
    uint64_t nphase    = atoi(argv[5]);
    uint64_t colsBlock = columns/blocks;

    omp_set_num_threads(nThreads);

    printf("Arguments: columns=%lu, blocks=%lu,reps=%d, nThreads=%lu, colsBlock=%lu, nphase=%lu, SIZE=%lu\n",columns, blocks, reps, nThreads,colsBlock,nphase,SIZE);

    #pragma omp parallel
    {
	#pragma omp master
	{
           printf("Using %d threads\n",omp_get_num_threads());    
	}
    }

    uint64_t length = SIZE;
    printf("Columns per proc=%lu(-1) reps=%d\n",columns,reps);
    Goldilocks g1(length, nThreads);
    
    uint64_t *pol_ext_1 = (uint64_t *)malloc(length * sizeof(uint64_t) *columns);
    uint64_t *pol_ext_2 = (uint64_t *)malloc(length * sizeof(uint64_t) *columns);

    // INITIALIZATION NON-VECTORIZED
    #pragma omp parallel for
    for(uint64_t k=0; k< columns; k++){ 
	    uint64_t offset = k*length;;
	    pol_ext_1[offset] = 1+k;
	    pol_ext_1[offset+1] = 1+k;
        pol_ext_2[offset] = pol_ext_1[offset];
        pol_ext_2[offset+1] = pol_ext_1[offset+1];
	    for (uint64_t i = 2; i < length; i++)
	    {
	        pol_ext_1[offset+i] = g1.gl_add(pol_ext_1[offset+i-1], pol_ext_1[offset+i-2]);
            pol_ext_2[offset+i] = pol_ext_1[offset+i];
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
            u_int64_t offset = i*length;
            g1.ntt(pol_ext_1+offset, length, 1);
            g1.ntt(pol_ext_2+offset, length, nphase);

            //g1.ntt_(pol_ext_2+offset, length,nphase);
            //g1.intt(pol_ext_2+offset, length,nphase);


        }
    }
    double runtime1 = omp_get_wtime() - st;

    printf("NON-VECTORIZED // Columns: %lu, time:%f\n", columns, runtime1 / reps);
   
    //
    // CHECK RESULTS
    //
    #pragma omp parallel for
    for(uint64_t k=0; k< columns; k++){ 
	    uint64_t offset = k*length;
	    for (uint64_t i = 0; i < length; i++)
	    {
            printf("hola i=%lu val1=%lu val2=%lu\n",i,pol_ext_1[offset+i] % GOLDILOCKS_PRIME,pol_ext_2[offset+i]);
            /*if(pol_ext_1[offset+i] != pol_ext_2[offset+i]){
                printf("hola i=%lu val1=%lu val2=%lu\n",i,pol_ext_1[offset+i],pol_ext_2[offset+i]);
            }*/
	    }
    }
    
    free(pol_ext_1);
    free(pol_ext_2);
    return 0;
}

