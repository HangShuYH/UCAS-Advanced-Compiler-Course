extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int func() {
    int a;
    a = 30;
    return a;
}

void func2() {
    PRINT(-100);
}
int main() {
   int a;
   a = func();
   PRINT(a);
}