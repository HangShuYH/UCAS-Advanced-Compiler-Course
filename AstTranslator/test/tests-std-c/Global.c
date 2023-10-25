#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}
int a = 10 + (10 + 10)/10;
int main() {
    PRINT(a);
    int b = -5;
    PRINT(b);
    b = a + b;
    PRINT(b);
}