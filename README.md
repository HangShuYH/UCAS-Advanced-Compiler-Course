## UCAS Advanced Compiler Course

### Lab1 AstTranslator

#### Requirements

Requirements: Implement a basic interpreter based on Clang

Marking: 25 testcases are provided and each test case counts for 1 mark

Supported Language: We support a subset of C language constructs, as follows: 

Type: int | char | void | *

Operator: * | + | - | * | / | < | > | == | = | [ ] 

Statements: IfStmt | WhileStmt | ForStmt | DeclStmt 

Expr : BinaryOperator | UnaryOperator | DeclRefExpr | CallExpr | CastExpr 

We also need to support 4 external functions int GET(), void * MALLOC(int), void FREE (void *), void PRINT(int), the semantics of the 4 funcions are self-explanatory. 

A skelton implemnentation ast-interpreter.tgz is provided, and you are welcome to make any changes to the implementation. The provided implementation is able to interpreter the simple program like : 

extern int GET();

extern void * MALLOC(int);

extern void FREE(void *);

extern void PRINT(int);

int main() {

   int a;

   a = GET();

   PRINT(a);
}

We provide a more formal definition of the accepted language, as follows: 


Program : DeclList

DeclList : Declaration DeclList | empty

Declaration : VarDecl FuncDecl

VarDecl : Type VarList;

Type : BaseType | QualType

BaseType : int | char | void

QualType : Type * 

VarList : ID, VarList |  | ID[num], VarList | emtpy

FuncDecl : ExtFuncDecl | FuncDefinition

ExtFuncDecl : extern int GET(); | extern void * MALLOC(int); | extern void FREE(void *); | extern void PRINT(int);

FuncDefinition : Type ID (ParamList) { StmtList }

ParamList : Param, ParamList | empty

Param : Type ID

StmtList : Stmt, StmtList | empty

Stmt : IfStmt | WhileStmt | ForStmt | DeclStmt | CompoundStmt | CallStmt | AssignStmt | 

IfStmt : if (Expr) Stmt | if (Expr) Stmt else Stmt

WhileStmt : while (Expr) Stmt

DeclStmt : Type VarList;

AssignStmt : DeclRefExpr = Expr;

CallStmt : CallExpr;

CompoundStmt : { StmtList }

ForStmt : for ( Expr; Expr; Expr) Stmt

Expr : BinaryExpr | UnaryExpr | DeclRefExpr | CallExpr | CastExpr | ArrayExpr | DerefExpr | (Expr) | num

BinaryExpr : Expr BinOP Expr

BinaryOP : + | - | * | / | < | > | ==

UnaryExpr : - Expr

DeclRefExpr : ID

CallExpr : DeclRefExpr (ExprList)

ExprList : Expr, ExprList | empty

CastExpr : (Type) Expr

ArrayExpr : DeclRefExpr [Expr]

DerefExpr : * DeclRefExpr

#### build/run 

build
```shell
cd AstTranslator
mkdir build
cd build
cmake -DCMAKE_LLVM_DIR=${YOUR_LLVM_DIR} ..
make 
```
runTest
```shell
python3 run_test.py -i tests
```

### Lab2

#### Requirements
output format：

${line} : ${func_name1}, ${func_name2}


${line} is unique in output.


Given the test file:
```c
  1 #include <stdio.h>
  2 
  3 int add(int a, int b) {
  4    return a+b;
  5 }
  6 
  7 int sub(int a, int b) {
  8    return a-b;
  9 }
 10 
 11 int foo(int a, int b, int (*a_fptr)(int, int)) {
 12     return a_fptr(a, b);
 13 }
 14 
 15 
 16 int main() {
 17     int (*a_fptr)(int, int) = add;
 18     int (*s_fptr)(int, int) = sub;
 19     int (*t_fptr)(int, int) = 0;
 20 
 21     char x;
 22     int op1, op2;
 23     fprintf(stderr, "Please input num1 +/- num2 \n");
 24 
 25     fscanf(stdin, "%d %c %d", &op1, &x, &op2);
 26 
 27     if (x == '+') {
 28        t_fptr = a_fptr;
 29     }
 30 
 31     if (x == '-') {
 32        t_fptr = s_fptr;
 33     }
 34 
 35     if (t_fptr != NULL) {
 36        unsigned result = foo(op1, op2, t_fptr);
 37        fprintf (stderr, "Result is %d \n", result);
 38     } else {
 39        fprintf (stderr, "Unrecoganised operation %c", x);
 40     }
 41     return 0;
 42 }
```
The output should be :

12 : add, sub

23 : fprintf

25 : fscanf

36 : foo

37 : fprintf

39 : fprintf
