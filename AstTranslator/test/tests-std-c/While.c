#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int main() {
   int a;
   a = 10;
   int i;
   i = 0;
   while(i < a) {
    PRINT(i);
    i = i +1;
   }
}