extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);
int a;
int *b;
int main() {
   a = GET();
   PRINT(a);
   b = &a;
   PRINT(*b);
   int a;
   a = 10;
   PRINT(a);
   b = &a;
   PRINT(*b);
   *b = 20;
   PRINT(a);
}