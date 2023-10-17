extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);
int main() {
   int a;
   a = 10;
   {
        int a;
        a = 20;
        PRINT(a);
   }
   PRINT(a);
}