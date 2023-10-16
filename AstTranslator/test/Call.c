extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int func() {
    int a;
    a = GET();
    return a;
}

int main() {
   int a;
   a = func();
   PRINT(a);
}