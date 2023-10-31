#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}


void swap(char *a, char *b) {
   char temp;
   temp = *a;
   *a = *b;
   *b = temp;
}

int main() {
   char* a; 
   char* b;
   a = (char *)MALLOC(1);
   b = (char *)MALLOC(1);
   
   *b = 24;
   *a = 42;

   swap(a, b);

   PRINT((int)*a);
   PRINT((int)*b);
   FREE(a);
   FREE(b);
   return 0;
}

