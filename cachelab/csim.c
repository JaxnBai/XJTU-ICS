#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <stdlib.h>
struct cache_line {
    int valid;
    int tag;
};
struct queue{
    int front;
    int rear;
};
int main(int argc, char** argv)
{
    int opt,s,E,b;
    char t[100]={0};
    while(-1 != (opt = getopt(argc, argv, "s:E:b:t:"))){
        switch(opt) {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
            int i=0;
               for(i=0;optarg[i];i++)
               t[i]=optarg[i];   
               t[i]=0;          
                break;
            default:
                printf("rong argument\n");
                break;
        }
    }
    const long long int S=1<<s,e=E+1;
    struct cache_line** cache=(struct cache_line**)malloc(S*sizeof(struct cache_line*));
    for(int i=0;i<S;i++)
   {
            cache[i]=(struct cache_line*)malloc(e*sizeof(struct cache_line));
        for(int j=0;j<e;j++)
        {
            cache[i][j].tag= 0xffffffff;
            cache[i][j].valid=0;
        }
   }
   struct queue* q=(struct queue*)malloc(S*sizeof(struct queue));
   for(int i=0;i<S;i++)
   {
    q[i].rear=0;
    q[i].front=0;
   }
    FILE * f;  //pointer  to  FILE  object
    f = fopen(t,"r");  //open file for reading
    char identifier;
    unsigned address;
    int size;
    int hit=0,miss=0,ev=0;
    // Reading lines like " L 10,1" or " M 20,1"
    while(fscanf(f," %c  %x,%d",  &identifier, &address, &size)>0) {//设立cache[S][e](e=E+1),q[S]
        if(identifier=='I')
        continue;
        int position_s= (address>>b)%(1<<s);//取出组索引s
        int position_e=address>>(s+b);//取出标记
        for(int i=q[position_s].front;   1  ;i=(i+1)%e)
        {
                if(cache[position_s][i].tag==position_e&&cache[position_s][i].valid)//hit的情况
                {
                    hit++;
                    break;
                }
                if(i==q[position_s].rear)//miss的情况
                {
                    miss++;
                     cache[position_s][q[position_s].rear].tag=position_e;
                        cache[position_s][q[position_s].rear].valid=1;
                        q[position_s].rear=(q[position_s].rear+1)%e;//入队，因为e=E+1，所以不会冲突
                    if(q[position_s].rear==q[position_s].front)//相等时队列有E+1个元素
                    {
                         cache[position_s][q[position_s].front].valid=0;
                       q[position_s].front=(q[position_s].front+1)%e;
                        ev++;
                    }
                   break;
                }
        }
        if(identifier=='M')
        hit++;
    }
    printSummary(hit,miss,ev);
     for(int i=0;i<S;i++)
           free(cache[i]);
        free(cache);
        free(q);
    return 0;
}
