#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int f(int x) {
  int i = 0;
  while (i < x) {
     i = i + 2;
  }
  return i + 10;
}
int main() {
   int a;
   int b;
   a = 1;
   b = f(a);
   PRINT(b);
}
