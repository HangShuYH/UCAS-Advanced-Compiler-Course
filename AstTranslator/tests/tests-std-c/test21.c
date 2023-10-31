#include <stdio.h>
#include <stdlib.h>
int GET() {int a;scanf("%d", &a); return a;}
void PRINT(int a) {fprintf(stderr,"%d", a);}
void* MALLOC(int a) {return (void*)malloc(a);}
void FREE(void * a) {free(a);}

void print1(int);
void print2(int);
   
void  print1(int x)
{
    if (x > 3) { 
    	PRINT(x-1);
	print2(x-3);
    }
    PRINT(x);
}

void print2(int b)
{
     if (b > 2) {
	 PRINT(b);
	 print1(b-2);
     }
}



  
   
int main() {
   int a[10];
   int i;

   a[0] = 0;
   a[1] = 1;
   for (i=2; i<10; i=i+1)
	a[i] = a[i-1]+a[i-2];
   print1(a[9]);
   return 0;
}
