#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}


void swap(char *a, char *b, int x) {
   char temp;	
   temp = *a;
   *a = *b;
   *b = temp;

   if (x >= 5) {
     swap(a, b, x-2);
   }
   else if (x >=2)
   {
      swap(a, b, x-1);
   }
}

void dswap(char **a, char **b, int x)
{
   swap(*a, *b, x);
}

int main() {
   char* a; 
   char **pa;
   char* b;
   char **pb;
   a = (char *)MALLOC(1);
   b = (char *)MALLOC(1);
   pa = (char **)MALLOC(8);
   pb = (char **)MALLOC(8);
   
   *b = 24;
   *a = 42;

   *pa = a;
   *pb = b;

   dswap(pa, pb, 6);

   PRINT((int)*a);
   PRINT((int)*b);
   FREE(a);
   FREE(b);
   return 0;
}

