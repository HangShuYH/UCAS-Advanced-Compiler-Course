extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = GET();
   PRINT(a);
   int b;
   b = a;
   PRINT(b);
}