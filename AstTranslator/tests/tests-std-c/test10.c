#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int main() {
   int a = 0;
   int b = 0;
   while (a < 10) {
      a = a + 1; 

      if (a > 5) b = b + 1;
   }
   PRINT(b);
}

//5