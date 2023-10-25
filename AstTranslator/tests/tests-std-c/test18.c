#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int main() {
   int* a;
   int **b;
   int *c;
   a = (int*)MALLOC(sizeof(int)*2);
   b = (int **)MALLOC(sizeof(int *));

   *b = a;
   *a = 10;
   *(a+1) = 20;

   c = *b;
   PRINT(*c);
   PRINT(*(c+1));
   FREE(a);
   FREE((int *)b);
}
//1020