#include "poly.h"
#include<time.h>
void poly_optim(const double a[], double x, long degree, double *result) {
     long i;
    double r = a[degree];
    double t1=0,t2=0,t3=0,t4=0,t5=0,t6=0,t7=0;
    double x8= x * x * x * x* x* x* x* x;
    double x7=x * x * x * x* x* x* x;
    double x6=x * x * x * x* x* x;
    double x5=x * x * x * x* x;
    double x4=x*x*x*x;
    double x3=x*x*x;
    double x2=x*x;
     for (i = degree - 1; i - 7 >= 0; i = i - 8) {
        t1=a[i]  +t1 * x8;
        t2=a[i-1]  +t2 * x8;
        t3=a[i-2]  +t3 * x8;
        t4=a[i-3]  +t4 * x8;
        t5=a[i-4]  +t5 * x8;
        t6=a[i-5]  +t6 * x8;
        t7=a[i-6]  +t7 * x8;
        r = a[i - 7] + r * x8;
    }
    *result = t1*x7+t2*x6+t3*x5+t4*x4+t5*x3+t6*x2+t7*x+r;
    for (; i >= 0; i--)
       *result=(*result) * x + a[i];
    
    return;
}

void measure_time(poly_func_t poly, const double a[], double x, long degree,
                  double *time) {
   long long int sum=1000;
   double res=0,temp=0;
   clock_t start,finish;
   start=clock();
   for(int i=0;i<sum;i++)  
   {
    poly(a,x,degree,&temp);
    
   }
   finish=clock();
    res+=(double)(finish-start);
    *time = ((double)(res / sum) / CLOCKS_PER_SEC)*1e9;
   return;
}