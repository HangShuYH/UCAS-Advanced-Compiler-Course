#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int b=10;
int f(int x,int y) {
  if (y > 0) 
  	return x + f(x,y-1);
  else 
    return 0;
}
int main() {
   int a=2;
   PRINT(f(b,a));
}


#20