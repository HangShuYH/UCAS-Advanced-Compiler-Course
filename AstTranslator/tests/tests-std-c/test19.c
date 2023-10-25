#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int main() {
   int* a;
   int* b;
   int* c[2];
   a = (int*)MALLOC(sizeof(int)*2);

   *a = 10;
   *(a+1) = 20;
   c[0] = a;
   c[1] = a+1;

   PRINT(*c[0]);
   PRINT(*c[1]);
}
