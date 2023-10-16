extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = GET();
   int b;
   b = GET();
   PRINT(a + b);
   PRINT(a - b);
   PRINT(a * b);
   PRINT(a / b);
   PRINT(a < b);
   PRINT(a > b);
   PRINT(a == b);
}