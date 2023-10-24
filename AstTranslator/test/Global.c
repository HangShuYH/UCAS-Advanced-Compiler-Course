extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);
int a = 10 + (10 + 10)/10;
int main() {
    PRINT(a);
    int b = -5;
    PRINT(b);
    b = a + b;
    PRINT(b);
}