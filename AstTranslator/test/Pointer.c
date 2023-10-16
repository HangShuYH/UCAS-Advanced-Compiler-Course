extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = GET();
   int *b;
   b = &a;
   PRINT(*b);
   *b = GET();
   PRINT(a);
   PRINT(*b);
}