#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int b=10;
int f(int x) {
  if (x > 0) 
  	return 5 + f(x - 5);
  else 
    return 0;
}
int main() {
   PRINT(f(b));
}


#10