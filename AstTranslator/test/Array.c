extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a[5];
   for (int i = 0;i < 5; i = i + 1) {
        a[i] = i;
   }
    for (int i = 0;i < 5; i = i + 1) {
        PRINT(a[i]);
   }
}