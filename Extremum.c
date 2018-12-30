#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

#define QUEUE_SIZE 1000
#define STACK_SIZE 1000
#define DATA_SIZE 200000

double static f(double x){
  int i;
  int j;
  double temp;
  double result = 0;
  double data[100];
  for(j = 100; j > 0; j--){
    data[j-1] = pow(x + 0.5*j, -3.3);
  }
  for(i = 100; i > 0; i--){
    temp = 0;
    for(j = i; j > 0; j--){
      temp += data[j-1];
    }
    result += sin(x + temp)/pow(1.3, (double)i);
  }
  return result;
}

/*
double static f(double x){
	return (x-10.0)*(x-10.0);
}
*/

int main(int argc, char** argv){

  int NUM_THREAD;
  int OVER;
  int search = 1;
  sscanf(argv[argc-1], "%d", &NUM_THREAD);
  sscanf(argv[argc-2], "%d", &OVER);
  sscanf(argv[argc-3], "%d", &search);
  printf("NUM_THREAD is %d, OVER is %d.\n", NUM_THREAD, OVER);

  double a = 1.0;
  double b = 100.0;
  double ep = 1e-6;
  double s = 12;

  double fa = f(a);
  double fb = f(b);
  double maximum = fa > fb ? fa : fb;

  double l; // local maximum
  double t; // top value
  double le; // left interval value
  double ri; // right interval value

  double queue[QUEUE_SIZE][2] = { {a,b} };
  double stack[STACK_SIZE][2];
  double data[DATA_SIZE][2];
  int queue_head = 0;
  int queue_tail = 1;
  int num_thread;
  int i;
  int p; // stack pointer
  int s1 = 0; // index for search f(x)
  int s2; // index for search f(x)

  double start_time = omp_get_wtime();

  if(search > 0){
    // create queue stores initial intervals in parallel
    while(queue_tail - queue_head <= OVER*NUM_THREAD){
      num_thread = queue_tail - queue_head < NUM_THREAD ?
                queue_tail - queue_head : NUM_THREAD;
      #pragma omp parallel num_threads(num_thread)
      {
        #pragma omp for schedule(dynamic, OVER) private(l,t,le,ri)
        for(i = queue_head; i < queue_tail; i++){
          le = 0;
          ri = 0;

          for(s2 = 0; s2 < s1; s2++){
            if(data[s2][0] == queue[i][0]){le = data[s2][1];}
            if(data[s2][0] == queue[i][1]){ri = data[s2][1];}
          }

          if(le == 0 && ri != 0){
            le = f(queue[i][0]);
            #pragma omp critical update
            {
              data[s1][0] = queue[i][0];
              data[s1][1] = le;
              s1 = (s1+1)%DATA_SIZE;
            }
          }
          else if(le != 0 && ri == 0){
            ri = f(queue[i][1]);
            #pragma omp critical update{
              data[s1][0] = queue[i][1];
              data[s1][1] = ri;
              s1 = (s1+1)%DATA_SIZE;
            }
          }
          else if(le == 0 && ri == 0){
            le = f(queue[i][0]);
            ri = f(queue[i][1]);
            #pragma omp critical update{
              data[s1][0] = queue[i][0];
              data[s1][1] = le;
              s1 = (s1+1)%DATA_SIZE;
              data[s1][0] = queue[i][1];
              data[s1][1] = ri;
              s1 = (s1+1)%DATA_SIZE;
            }
          }

          l = le > ri ? le : ri;
          //printf("maximum is %f, local is %f.\n", maximum, l);

          if(l > maximum){
            #pragma omp critical
            maximum = l;
          }

          t = (le+ri+s*(queue[i][1]-queue[i][0]))/2;

          #pragma omp critical
          {
            if(maximum+ep <= t){
              queue[queue_tail][0] = queue[i][0];
              queue[queue_tail++][1] = (queue[i][0] + queue[i][1])/2;
              queue[queue_tail][0] = queue[queue_tail-1][1];
              queue[queue_tail++][1] =  queue[i][1];
              queue_head++;
            }else{
              queue_head++;
            }
          }
        }
      }
    }

    /*
    int j = queue_head;
    for(; j < queue_tail; j++){
      printf("queue index0 %d value0 %f value1 %f.\n",j,queue[j][0], queue[j][1]);
    }
    */

    #pragma omp parallel num_threads(NUM_THREAD)
    {
      #pragma omp for schedule(dynamic,OVER) private(stack,p,le,ri,l,t,data,s1,s2)
      for(i = queue_head; i < queue_tail; i++){
        p = 0;
        stack[p][0] = queue[i][0];
        stack[p][1] = queue[i][1];
        s1 = 0;
        while(p >= 0){
          le = 0;
          ri = 0;
          for(s2 = 0; s2 < s1; s2++){
            if(data[s2][0] == stack[p][0]) {le = data[s2][1];}
            if(data[s2][0] == stack[p][1]) {ri = data[s2][1];}
          }

          if(le!=0 && ri==0){
            ri = f(stack[p][1]);
            data[s1][0] = stack[p][1];
            data[s1][1] = ri;
            s1 = (s1+1)%DATA_SIZE;
          }
          else if(le==0 && ri!=0){
            le = f(stack[p][0]);
            data[s1][0] = stack[p][0];
            data[s1][1] = le;
            s1 = (s1+1)%DATA_SIZE;
          }
          else if(le==0 && ri==0){
            le = f(stack[p][0]);
            ri = f(stack[p][1]);
            data[s1][0] = stack[p][0];
            data[s1][1] = le;
            s1 = (s1+1)%DATA_SIZE;
            data[s1][0] = stack[p][1];
            data[s1][1] = ri;
            s1 = (s1+1)%DATA_SIZE;
          }
          l = le > ri ? le : ri ;

          if(l > maximum){
            #pragma omp critical
            maximum = l;
          }
          t = (le + ri + s*(stack[p][1]-stack[p][0]))/2;

          if(maximum + ep < t){
            stack[p][1] = (stack[p][0] + stack[p][1])/2;
            p++;
            stack[p][0] = stack[p-1][1];
            stack[p][1] = 2*stack[p-1][1]-stack[p-1][0];
          }else{
            p--;
          }
        }
      }
    }
  }
  else{
    // create queue stores initial intervals in parallel
    while(queue_tail - queue_head <= OVER*NUM_THREAD){
      num_thread = queue_tail - queue_head < NUM_THREAD ?
                queue_tail - queue_head : NUM_THREAD;
      #pragma omp parallel num_threads(num_thread)
      {
        #pragma omp for schedule(dynamic, OVER) private(l,t,le,ri)
        for(i = queue_head; i < queue_tail; i++){
          le = f(queue[i][0]);
          ri = f(queue[i][1]);

          l = le > ri ? le : ri;
          //printf("maximum is %f, local is %f.\n", maximum, l);

          if(l > maximum){
            #pragma omp critical
            maximum = l;
          }

          t = (le+ri+s*(queue[i][1]-queue[i][0]))/2;

          #pragma omp critical
          {
            if(maximum+ep <= t){
              queue[queue_tail][0] = queue[i][0];
              queue[queue_tail++][1] = (queue[i][0] + queue[i][1])/2;
              queue[queue_tail][0] = queue[queue_tail-1][1];
              queue[queue_tail++][1] =  queue[i][1];
              queue_head++;
            }else{
              queue_head++;
            }
          }
        }
      }
    }

    /*
    int j = queue_head;
    for(; j < queue_tail; j++){
      printf("queue index0 %d value0 %f value1 %f.\n",j,queue[j][0], queue[j][1]);
    }
    */

    #pragma omp parallel num_threads(NUM_THREAD)
    {
      #pragma omp for schedule(dynamic,OVER) private(stack,p,le,ri,l,t)
      for(i = queue_head; i < queue_tail; i++){
        p = 0;
        stack[p][0] = queue[i][0];
        stack[p][1] = queue[i][1];

        while(p >= 0){
          le = f(stack[p][0]);
          ri = f(stack[p][1]);
          l = le > ri ? le : ri ;
          if(l > maximum){
            #pragma omp critical
            maximum = l;
          }
          t = (le + ri + s*(stack[p][1]-stack[p][0]))/2;

          if(maximum + ep < t){
            stack[p][1] = (stack[p][0] + stack[p][1])/2;
            p++;
            stack[p][0] = stack[p-1][1];
            stack[p][1] = 2*stack[p-1][1]-stack[p-1][0];
          }else{
            p--;
          }
        }
      }
    }
  }
  double end_time = omp_get_wtime();
  printf("maximum val %f, run time %f.\n", maximum, end_time - start_time);
  return 0;
}
