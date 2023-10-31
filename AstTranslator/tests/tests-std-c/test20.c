#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

   
int  fibonacci(int b) {
   int i;
   int c;
   int a[2];

   if (b < 2)
      return b;
   for (i=0; i< 2; i=i+1)
   {
       a[i] = b-1-i;
   }
   c = fibonacci(a[0]) + fibonacci(a[1]);
   return c;
}  
   
int main() {
   int a;
   int b;
   a = 5;

   b = fibonacci(5);
   PRINT(b);
   return 0;
}
