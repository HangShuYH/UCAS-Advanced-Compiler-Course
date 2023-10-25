#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int b;
int f(int x) {
  return x + 10;
}
int main() {
   int a;
   int b;
   a = -10;
   if (a > 0) 
      b = f(a);
   else 
      b = f(-a);
   PRINT(b);
}
