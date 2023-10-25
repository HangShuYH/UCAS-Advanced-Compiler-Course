#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int main() {
   int a[5];
   for (int i = 0;i < 5; i = i + 1) {
        a[i] = i;
   }
    for (int i = 0;i < 5; i = i + 1) {
        PRINT(a[i]);
   }
}