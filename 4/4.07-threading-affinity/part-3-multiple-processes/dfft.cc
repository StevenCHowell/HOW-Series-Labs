


#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mkl.h>
#include <mkl_dfti.h>
#include <omp.h>

int main(int argc, char* argv[]) {

  long n;
  if (argc>1)
    n = atoi(argv[1]);
  else
    n = 1<<26;

  double* X=(double*)_mm_malloc(n*sizeof(double), 64);

  DFTI_DESCRIPTOR_HANDLE fftHandle;
  MKL_LONG size = n;
  DftiCreateDescriptor (&fftHandle, DFTI_SINGLE, DFTI_REAL, 1, size);
  DftiCommitDescriptor (fftHandle);

  const double HztoPerf = 1e-9*double(2.5*n*log2(double(n)));

  const int nTrials=100;
  const int skipTrials=3;
  double rate=0, dRate=0;

  printf("\n\033[1mBenchmarking DFFT.\033[0m\n");
  printf("Problem size: %d (%.3f GB)\n",
	 n, double(n*sizeof(double))*1e-9);
  printf("    Platform: %s\n",
#ifndef __MIC__
	 "CPU"
#else
	 "MIC"
#endif
	 );
  printf("     Threads: %d\n", omp_get_max_threads());
  printf("    Affinity: %s\n\n", getenv("KMP_AFFINITY"));

  // Initializing data
#pragma omp parallel for
  for (int i = 0; i < n*n; i++) {
    X[i] = 0.0;
  }

  printf("\033[1m%5s %10s %15s\033[0m\n", "Trial", "Time, s", "Perf, GFLOP/s");

  for (int trial = 1; trial <= nTrials; trial++) {
    const double tStart = omp_get_wtime();
    DftiComputeForward (fftHandle, X);
    const double tEnd = omp_get_wtime();

    if ( trial > skipTrials) { // First two iterations are slow on Xeon Phi; exclude them
      rate  += HztoPerf/(tEnd - tStart); 
      dRate += HztoPerf*HztoPerf/((tEnd - tStart)*(tEnd-tStart)); 
    }

    if ((trial==1) || (trial%10==0))
      printf("%5d %10.3e %15.2f %s\n", 
	     trial, (tEnd-tStart), HztoPerf/(tEnd-tStart), (trial<=skipTrials?"*":""));
    fflush(stdout);
  }

  rate/=(double)(nTrials-skipTrials); 
  dRate=sqrt(dRate/(double)(nTrials-skipTrials)-rate*rate);
  printf("-----------------------------------------------------\n");
  printf("\033[1m%s %4s \033[42m%10.2f +- %.2f GFLOP/s\033[0m\n",
	 "Average performance:", "", rate, dRate);
  printf("-----------------------------------------------------\n");
  printf("* - warm-up, not included in average\n\n");


  DftiFreeDescriptor (&fftHandle);
  _mm_free(X);

}
