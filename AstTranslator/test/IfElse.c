extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = 50;
   if (a > 0) 
    PRINT(a);
   else {
    PRINT(-a);
   }
}