//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool
//--------------===//
//===----------------------------------------------------------------------===//
#ifndef __ASTINTERPRETER_H
#define __ASTINTERPRETER_H
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/LLVM.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

class Environment;

class InterpreterVisitor : public EvaluatedExprVisitor<InterpreterVisitor> {
public:
  explicit InterpreterVisitor(const ASTContext &context, Environment *env)
      : EvaluatedExprVisitor(context), mEnv(env) {}
  virtual ~InterpreterVisitor() {}

  virtual void VisitBinaryOperator(BinaryOperator *bop);
  virtual void VisitDeclRefExpr(DeclRefExpr *expr);
  virtual void VisitCastExpr(CastExpr *expr);
  virtual void VisitCallExpr(CallExpr *call);
  virtual void VisitDeclStmt(DeclStmt *declstmt);
  virtual void VisitUnaryOperator(UnaryOperator *unaryOperator);
  virtual void VisitIfStmt(IfStmt *ifstmt);
  virtual void VisitForStmt(ForStmt *forStmt);
  virtual void VisitWhileStmt(WhileStmt *whileStmt);
private:
  Environment *mEnv;
};

#endif