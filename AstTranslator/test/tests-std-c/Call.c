#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int func() {
    int a;
    a = 30;
    return a;
}

void func2() {
    PRINT(-100);
}
int main() {
   int a;
   a = func();
   PRINT(a);
}