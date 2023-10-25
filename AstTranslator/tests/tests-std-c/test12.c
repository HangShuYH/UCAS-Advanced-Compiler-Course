#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

int main() {
	int a[3];
        int i;
	int b = 0;
        for (i=0; i<3; i = i + 1) {
           a[i] = i * i;
        }
        b = a[2];
	PRINT(b);
}
