extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = 50;
   for (int i = 0;i < a; i = i + 1) {
    PRINT(i);
   }
}