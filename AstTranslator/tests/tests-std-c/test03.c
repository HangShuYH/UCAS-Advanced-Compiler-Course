#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int main() {
   int a;
   int b = 10;
   a = 10;
   if (a == 10) {
     b = 20;
   } else {
     b = 0;
   }
   PRINT(a*b);
}


// 200