extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = 5;
   int b;
   b = 10;
   PRINT(a + b);
   PRINT(a - b);
   PRINT(a * b);
   PRINT(a / b);
   PRINT(a < b);
   PRINT(a > b);
   PRINT(a == b);
}