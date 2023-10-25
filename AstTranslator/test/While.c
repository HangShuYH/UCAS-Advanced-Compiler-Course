extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = 10;
   int i;
   i = 0;
   while(i < a) {
    PRINT(i);
    i = i +1;
   }
}