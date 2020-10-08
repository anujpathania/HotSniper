// This code was based on
// http://software.intel.com/en-us/articles/writing-parallel-programs-a-multi-language-tutorial-introduction/

#include "omp.h"
#include "stdio.h"
#include "hooks_base.h"

long num_steps;
double step;

void main(int argc, char* argv[])
{
   int i;
   double x, pi, sum = 0.0;

   if (argc > 1)
      num_steps = atoi(argv[1]);
   else
      num_steps = 100000;

   step = 1.0/(double) num_steps;

   parmacs_roi_begin();

   #pragma omp parallel for private(x) reduction(+:sum)
   for (i=0;i<= num_steps; i++)
   {
      x = (i+0.5)*step;
      sum = sum + 4.0/(1.0+x*x);
   }
   pi = step * sum;

   parmacs_roi_end();

   printf("%lf\n", pi);
}
