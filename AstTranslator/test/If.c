extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = GET();
   if (a > 0) 
    PRINT(a);
}