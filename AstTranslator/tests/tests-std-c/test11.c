#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int main() {
   int a, b, c;
   a = 0;
   c = 0;
   while (a < 10) {
      a = a + 1;
      b = 0;
      while (b < 10) {
          b = b + 1;
          c = c + 1;
      }
   }
   PRINT(c);
}

//100