//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include <cassert>
#include <map>
#include <stdio.h>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtIterator.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

class Value {
	int intVal;
	Decl* addrVal;
	std::vector<int> arrayVal;
	enum ValueType{IntType,AddrType,ArrayType}type;

public:
	Value(int val = 0):intVal(val), type(IntType) {}
	explicit Value(Decl* val):addrVal(val), type(AddrType) {}
	explicit Value(std::vector<int> val):arrayVal(val), type(ArrayType) {}
	bool isIntType() const {return type == IntType;}
	bool isAddrType() const {return type == AddrType;}
	bool isArrayType() const {return type == ArrayType;}
	void setArrayType() {type = ArrayType;}
	int getInt() const {return intVal;}
	Decl* getAddr() const {return addrVal;}
	void setArrayVal(int idx, int val) {
		assert(type == ArrayType);
		assert(idx < arrayVal.size());
		arrayVal[idx] = val;
	}
	int getArrayVal(int idx) const {
		assert(type == ArrayType);
		assert(idx < arrayVal.size());
		return arrayVal[idx];
	}
};

class StackFrame {
   /// StackFrame maps Variable Declaration to Value
   /// Which are either integer or addresses (also represented using an Integer value)
   std::map<Decl*, Value> mVars;
   std::map<Stmt*, Value> mExprs;

   std::map<Stmt*, Value> mAddrs;
   std::map<Stmt*, int> mCursors;
   /// The current stmt
   Stmt * mPC;
public:
   StackFrame() : mVars(), mExprs(), mPC(), mAddrs() {
   }
   
   void bindAddr(Stmt* stmt, Value val) {
	  mAddrs[stmt] = val;
   }
   void bindCursor(Stmt* stmt, int idx) {
	  mCursors[stmt] = idx;
   }
   int getCursor(Stmt* stmt) {
	  return mCursors[stmt];
   }
   bool hasCursor(Stmt* stmt) {
	  return mCursors.find(stmt) != mCursors.end();
   }
   Value getAddr(Stmt* stmt) {
	  return mAddrs[stmt];
   }
   void bindDecl(Decl* decl, Value val) {
      mVars[decl] = val;
   }    
   bool hasDeclVal(Decl* decl) {
	  return mVars.find(decl) != mVars.end();
   }
   Value& getDeclVal(Decl * decl) {
      assert (mVars.find(decl) != mVars.end());
      return mVars.find(decl)->second;
   }
   void bindStmt(Stmt * stmt, Value val) {
	   mExprs[stmt] = val;
   }
   Value getStmtVal(Stmt * stmt, const ASTContext& Context) {
	   if (Expr* expr = llvm::dyn_cast<Expr>(stmt)) {
			Expr::EvalResult result;
			if (expr->EvaluateAsInt(result, Context)) {
				return result.Val.getInt().getExtValue();
			}
	   }
	   assert (mExprs.find(stmt) != mExprs.end());
	   return mExprs[stmt];
   }
   void setPC(Stmt * stmt) {
	   mPC = stmt;
   }
   Stmt * getPC() {
	   return mPC;
   }
};

/// Heap maps address to a value
/*
class Heap {
public:
   int Malloc(int size) ;
   void Free (int addr) ;
   void Update(int addr, int val) ;
   int get(int addr);
};
*/
class GlobalRegion {
   std::map<Decl *, Value> mVars;
public:
	void bindDecl(Decl* decl, Value val) {
		mVars[decl] = val;
	}
	Value& getDecl(Decl* decl) {
		assert(mVars.find(decl) != mVars.end());
		return mVars[decl];
	}
};

class Environment {
   std::vector<StackFrame> mStack;
   GlobalRegion mGBRegion;

   FunctionDecl * mFree;				/// Declartions to the built-in functions
   FunctionDecl * mMalloc;
   FunctionDecl * mInput;
   FunctionDecl * mOutput;

   FunctionDecl * mEntry;

   const ASTContext& mContext;
public:
   /// Get the declartions to the built-in functions
   Environment(const ASTContext& Context) : mContext(Context), mStack(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL) {
   }


   /// Initialize the Environment
   void init(TranslationUnitDecl * unit) {
	   for (TranslationUnitDecl::decl_iterator i =unit->decls_begin(), e = unit->decls_end(); i != e; ++ i) {
		   if (FunctionDecl * fdecl = dyn_cast<FunctionDecl>(*i) ) {
			   if (fdecl->getName().equals("FREE")) mFree = fdecl;
			   else if (fdecl->getName().equals("MALLOC")) mMalloc = fdecl;
			   else if (fdecl->getName().equals("GET")) mInput = fdecl;
			   else if (fdecl->getName().equals("PRINT")) mOutput = fdecl;
			   else if (fdecl->getName().equals("main")) mEntry = fdecl;
		   } else if (VarDecl* vdecl = dyn_cast<VarDecl>(*i)) {
			   mGBRegion.bindDecl(vdecl, Value(0));
		   }
	   }
	   mStack.push_back(StackFrame());
   }

   FunctionDecl * getEntry() {
	   return mEntry;
   }

   /// !TODO Support comparison operation
    void binop(BinaryOperator *bop) {
	    Expr * left = bop->getLHS();
	    Expr * right = bop->getRHS();

	    if (bop->isAssignmentOp()) {
		   Value val = mStack.back().getStmtVal(right, mContext);
		   mStack.back().bindStmt(left, val);
		   if (DeclRefExpr * declexpr = dyn_cast<DeclRefExpr>(left)) {
			   Decl * decl = declexpr->getFoundDecl();
			   mStack.back().bindDecl(decl, val);
		   } else {
			   Value value = mStack.back().getAddr(left);
			   Decl* decl = value.getAddr();
			   if (mStack.back().hasCursor(left)) {
					mStack.back().getDeclVal(decl).setArrayVal(mStack.back().getCursor(left), val.getInt());
			   } else {
			   		mStack.back().bindDecl(decl, val);
			   }
		   }
		   return;
	    }
		int leftVal = mStack.back().getStmtVal(left, mContext).getInt();
		int rightVal = mStack.back().getStmtVal(right, mContext).getInt();
		int resultVal;
		switch (bop->getOpcode()) {
			case clang::BO_Add:
				resultVal = leftVal + rightVal;
				break;
			case clang::BO_Sub:
				resultVal = leftVal - rightVal;
				break;
			case clang::BO_Mul:
				resultVal = leftVal * rightVal;
				break;
			case clang::BO_Div:
				assert(rightVal != 0);
				resultVal = leftVal / rightVal;
				break;
			case clang::BO_LT:
				resultVal = (leftVal < rightVal);
				break;
			case clang::BO_GT:
				resultVal = (leftVal > rightVal);
				break;
			case clang::BO_EQ:
				resultVal = (leftVal == rightVal);
				break;
			default:
				llvm::errs() << "Not Supportted Opcode in Binop!\n";
		}
		mStack.back().bindStmt(bop, resultVal);
    }

    void decl(DeclStmt * declstmt) {
	   for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
			   it != ie; ++ it) {
		   Decl * decl = *it;
		   if (VarDecl * vardecl = dyn_cast<VarDecl>(decl)) {
			   if (auto constantArrayType = dyn_cast<ConstantArrayType>(vardecl->getType().getTypePtr())) {
					std::vector<int> vec(constantArrayType->getSize().getSExtValue());
					mStack.back().bindDecl(vardecl, Value(vec));
			   } else {
			   		mStack.back().bindDecl(vardecl, 0);
			   }
		   }
		   
	   }
   }
   
   void declref(DeclRefExpr * declref) {
	   mStack.back().setPC(declref);
	   if (!declref->getType()->isFunctionType()) {
		   Decl* decl = declref->getFoundDecl();
		   if (declref->getType()->isArrayType()) {
			 	mStack.back().bindStmt(declref, Value(decl));
		   } else {
				Value val;
				if (mStack.back().hasDeclVal(decl)) {
					val = mStack.back().getDeclVal(decl);
				} else {
					val = mGBRegion.getDecl(decl);
				}
				mStack.back().bindStmt(declref, val);
		   }
	   }
   }

   void cast(CastExpr * castexpr) {
	   mStack.back().setPC(castexpr);
	   if (!castexpr->getType()->isFunctionPointerType()) {
		   Expr * expr = castexpr->getSubExpr();
		   Value val = mStack.back().getStmtVal(expr, mContext);
		   mStack.back().bindStmt(castexpr, val );
	   }
   }

   /// !TODO Support Function Call
   void call(CallExpr * callexpr) {
	   mStack.back().setPC(callexpr);
	   int val = 0;
	   FunctionDecl * callee = callexpr->getDirectCallee();
	   if (callee == mInput) {
		  llvm::errs() << "Please Input an Integer Value : ";
		  scanf("%d", &val);

		  mStack.back().bindStmt(callexpr, val);
	   } else if (callee == mOutput) {
		   Expr * decl = callexpr->getArg(0);
		   val = mStack.back().getStmtVal(decl, mContext).getInt();
		   llvm::errs() << val;
	   } else {
		   /// You could add your code here for Function call Return
		   
	   }
   }

   void unary(UnaryOperator* unaryOperator) {
	   mStack.back().setPC(unaryOperator);
	   Expr* subExpr = unaryOperator->getSubExpr();
	   switch (unaryOperator->getOpcode()) {
			default:
				llvm::errs() << "UnSupported UnaryOpcode!\n";
				break;
			case clang::UO_Minus:
				mStack.back().bindStmt(unaryOperator, 
	         -mStack.back().getStmtVal(subExpr,  mContext).getInt());
			 	break;
			case clang::UO_AddrOf:
				if (DeclRefExpr * declexpr = dyn_cast<DeclRefExpr>(subExpr)) {
			  		Decl * decl = declexpr->getFoundDecl();
			   		mStack.back().bindStmt(unaryOperator, Value(decl));
		   		}
				break;
			case clang::UO_Deref:
				mStack.back().bindStmt(unaryOperator, 
					mStack.back().getDeclVal(
						mStack.back().getStmtVal(subExpr, mContext).getAddr())
				);
				mStack.back().bindAddr(unaryOperator, mStack.back().getStmtVal(subExpr, mContext));
				break;
	   }
   }

   bool getCond(Stmt* cond) {
	  mStack.back().setPC(cond);
	  return mStack.back().getStmtVal(cond, mContext).getInt() != 0;
   }

   void arraySubscriptExpr(ArraySubscriptExpr* arraySubscriptExpr) {
	  mStack.back().setPC(arraySubscriptExpr);
	  int idx = mStack.back().getStmtVal(arraySubscriptExpr->getIdx(), mContext).getInt();
	  Decl* base = mStack.back().getStmtVal(arraySubscriptExpr->getBase(), mContext).getAddr();
	  Value value = mStack.back().getDeclVal(base);
	  mStack.back().bindStmt(arraySubscriptExpr, value.getArrayVal(idx));
	  mStack.back().bindAddr(arraySubscriptExpr, Value(base));
	  mStack.back().bindCursor(arraySubscriptExpr, idx);
   }
};


