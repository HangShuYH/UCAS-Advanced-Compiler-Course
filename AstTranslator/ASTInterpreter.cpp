//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include "Environment.h"
#include "ASTInterpreter.h"
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

#include "Environment.h"


void InterpreterVisitor::VisitBinaryOperator(BinaryOperator * bop) {
   VisitStmt(bop);
   mEnv->binop(bop);
}
void InterpreterVisitor::VisitDeclRefExpr(DeclRefExpr *expr) {
   VisitStmt(expr);
   mEnv->declref(expr);
}
void InterpreterVisitor::VisitCastExpr(CastExpr *expr) {
   VisitStmt(expr);
   mEnv->cast(expr);
}
void InterpreterVisitor::VisitCallExpr(CallExpr *call) {
   VisitStmt(call);
   mEnv->call(call);
}
void InterpreterVisitor::VisitDeclStmt(DeclStmt *declstmt) {
   mEnv->decl(declstmt);
}
void InterpreterVisitor::VisitUnaryOperator(UnaryOperator *unaryOperator) {
   VisitStmt(unaryOperator);
   mEnv->unary(unaryOperator);
}
void InterpreterVisitor::VisitIfStmt(IfStmt *ifstmt) {
   Expr* cond = ifstmt->getCond();
   Visit(cond);
   if (mEnv->getCond(cond)) {
      Visit(ifstmt->getThen());
   } else if (ifstmt->hasElseStorage()) {
      Visit(ifstmt->getElse());
   }
}
void InterpreterVisitor::VisitForStmt(ForStmt *forStmt) {
   if (forStmt->getInit())
      Visit(forStmt->getInit());
   while(true) {
      Expr* cond = forStmt->getCond();
      if (cond) {
         Visit(cond);
         if (mEnv->getCond(cond)) {
            Visit(forStmt->getBody());
         } else {
            break;
         }
      }
      if (forStmt->getInc())
         Visit(forStmt->getInc());
   }
}
void InterpreterVisitor::VisitWhileStmt(WhileStmt *whileStmt) {
   while(true) {
      Expr* cond = whileStmt->getCond();
      Visit(cond);
      if (mEnv->getCond(cond)) {
         Visit(whileStmt->getBody());
      } else {
         break;
      }
   }
}
void InterpreterVisitor::VisitReturnStmt(ReturnStmt* returnStmt) {
   VisitStmt(returnStmt);
   mEnv->ret(returnStmt);
}
void InterpreterVisitor::VisitUnaryExprOrTypeTraitExpr(
   UnaryExprOrTypeTraitExpr *unaryExprOrTypeTraitExpr) {
   VisitStmt(unaryExprOrTypeTraitExpr);
   mEnv->unaryExprOrTypeTraitExpr(unaryExprOrTypeTraitExpr);
}
void InterpreterVisitor::VisitParenExpr(ParenExpr *parenExpr) {
   VisitStmt(parenExpr);
   mEnv->parenExpr(parenExpr);
}
void InterpreterVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr* arraySubscriptExpr) {
   VisitStmt(arraySubscriptExpr);
   mEnv->arraySubscriptExpr(arraySubscriptExpr);
}

class InterpreterConsumer : public ASTConsumer {
public:
   explicit InterpreterConsumer(const ASTContext& context) : mEnv(context),
   	   mVisitor(context, &mEnv) {
   }
   virtual ~InterpreterConsumer() {}

   virtual void HandleTranslationUnit(clang::ASTContext &Context) {
	   TranslationUnitDecl * decl = Context.getTranslationUnitDecl();
	   mEnv.init(decl, &mVisitor);

	   FunctionDecl * entry = mEnv.getEntry();
	   mVisitor.VisitStmt(entry->getBody());
  }
private:
   Environment mEnv;
   InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public ASTFrontendAction {
public: 
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new InterpreterConsumer(Compiler.getASTContext()));
  }
};

int main (int argc, char ** argv) {
   if (argc > 1) {
       clang::tooling::runToolOnCode(std::unique_ptr<clang::FrontendAction>(new InterpreterClassAction), argv[1]);
   }
}
