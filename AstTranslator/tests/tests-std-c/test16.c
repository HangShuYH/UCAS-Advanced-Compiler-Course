#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int  x(int y) {
	return y + 10;
}

int  f(int b) {
   return x(b) + 10;
}

int main() {
   int a;
   a = 10;

   a = f(a);
   PRINT(a);
   return 0;
}

