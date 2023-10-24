//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#ifndef __ENVIRONMENT_H
#define __ENVIRONMENT_H
#include "ASTInterpreter.h"
#include <cassert>
#include <map>
#include <set>
#include <stdio.h>
#include <tuple>
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

class StackFrame {
   /// StackFrame maps Variable Declaration to Value
   /// Which are either integer or addresses (also represented using an Integer value)
   std::map<Decl*, int> mVars;
   std::map<Stmt*, int> mExprs;
   /// The current stmt
   Stmt * mPC;

   // for call
   std::vector<int> params;
   int retVal;
public:
   StackFrame() : mVars(), mExprs(), mPC() {
   }
   
   void pushParam(int val) {
	params.push_back(val);
   }
   std::vector<int> getParams() {
	return params;
   }
   void setRetVal(int val) {
	retVal = val;
   }
   int getRetVal() {
	return retVal;
   }

   void bindDecl(Decl* decl, int val) {
      mVars[decl] = val;
   }    
   bool getDeclVal(Decl * decl, int& val) {
      if (mVars.find(decl) == mVars.end()) {
		return false;
	  }
      val = mVars.find(decl)->second;
	  return true;
   }
   void bindStmt(Stmt * stmt, int val) {
	   mExprs[stmt] = val;
   }
   int getStmtVal(Stmt * stmt, const ASTContext& Context) {
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
class Heap {
private:
	class HeapItem {
	public:
		void* start;
		int size;
		HeapItem(){}
		HeapItem(void* _start, int _size):start(_start), size(_size){}
	};
	class CountAllocator {
	private:
		int maxCount;
	public:
		CountAllocator(int startCount = 0):maxCount(startCount){}
		int allocateCount(int i) {
			int addr = maxCount;
			maxCount += i;
			return addr;
		}
		void freeCount(int i) {
			//Trivial
		}
	};
private:
	std::map<int, HeapItem> items;
	CountAllocator counter;
public:
   int Malloc(int size) {
		void* start = static_cast<void*>(new char[size]);
		HeapItem heapItem(start, size);
		int idx = counter.allocateCount(size);
		items[idx] = heapItem;
		// llvm::errs() << "Malloc(" << idx << "), size: " << heapItem.size << "\n"; 
		return idx;
   }
   void Free(int itemIdx) {
		auto getRes = get(itemIdx);
	    HeapItem heapItem = std::get<0>(getRes);
		int idx = std::get<1>(getRes);
		assert(idx == 0);
		delete [] static_cast<char*>(heapItem.start);
		counter.freeCount(itemIdx);
   }
   void Update(int itemIdx, int val) {
	    auto getRes = get(itemIdx);
	    HeapItem heapItem = std::get<0>(getRes);
		int idx = std::get<1>(getRes);
		// llvm::errs() << "Update: " << idx << " " << heapItem.size << "\n";
		assert(idx * sizeof(int) < heapItem.size);
		*(static_cast<int*>(heapItem.start) + idx) = val;
   }
   int Get(int itemIdx) {
		auto getRes = get(itemIdx);
	    HeapItem heapItem = std::get<0>(getRes);
		int idx = std::get<1>(getRes);
		// llvm::errs() << "Get: " << idx << " " << heapItem.size << "\n";
		assert(idx * sizeof(int) < heapItem.size);
		return *(static_cast<int*>(heapItem.start) + idx);
   }
private:
   std::tuple<HeapItem, int> get(int itemIdx) {
		// for (auto p: items) {
		// 	llvm::errs() << p.first << " ";
		// }
		// llvm::errs() << "\n";
		auto it = items.upper_bound(itemIdx);
		if (it == items.begin()) {
			assert("Heap Address Not Found\n");
		}
		it--;
		// llvm::errs() << "itemIdx: " << itemIdx << ", it.idx: " << it->first << "\n";
        return std::make_tuple(it->second, itemIdx - it->first);
   }
};

class GlobalRegion {
	std::map<Decl*, int> globals;
public:
	void bindDecl(Decl* decl, int val) {
		globals[decl] = val;
	}
	bool getDecl(Decl* decl, int& val) {
		if (globals.find(decl) == globals.end()) {
			return false;
		}
		val = globals[decl];
		return true;
	}
};

class Environment {
   std::vector<StackFrame> mStack;

   FunctionDecl * mFree;				/// Declartions to the built-in functions
   FunctionDecl * mMalloc;
   FunctionDecl * mInput;
   FunctionDecl * mOutput;

   FunctionDecl * mEntry;

   const ASTContext& mContext;

   GlobalRegion globalRegion;

   Heap heap;

   InterpreterVisitor* visitor;

   //Temp variables, not useful for others
   int tempHeapAddr;
public:
   /// Get the declartions to the built-in functions
   Environment(const ASTContext& Context) : mContext(Context), mStack(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL) {
   }

   bool getDecl(Decl* decl,int& val) {
	 return mStack.back().getDeclVal(decl, val) || globalRegion.getDecl(decl, val);
   }
   /// Initialize the Environment
   void init(TranslationUnitDecl * unit, InterpreterVisitor* _visitor) {
		visitor = _visitor;
	   mStack.push_back(StackFrame());
	   for (TranslationUnitDecl::decl_iterator i =unit->decls_begin(), e = unit->decls_end(); i != e; ++ i) {
		   if (FunctionDecl * fdecl = dyn_cast<FunctionDecl>(*i) ) {
			   if (fdecl->getName().equals("FREE")) mFree = fdecl;
			   else if (fdecl->getName().equals("MALLOC")) mMalloc = fdecl;
			   else if (fdecl->getName().equals("GET")) mInput = fdecl;
			   else if (fdecl->getName().equals("PRINT")) mOutput = fdecl;
			   else if (fdecl->getName().equals("main")) mEntry = fdecl;
		   } else if (VarDecl* vdecl = dyn_cast<VarDecl>(*i)) {
			   int init = 0;
			   if (vdecl->hasInit()) {
				   visitor->Visit(vdecl->getInit());
				   init = mStack.back().getStmtVal(vdecl->getInit(), mContext);
			   }
			   globalRegion.bindDecl(vdecl, init);
		   }
	   }
	   mStack.pop_back();
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
		   int val = mStack.back().getStmtVal(right, mContext);
		   mStack.back().bindStmt(left, val);
		   if (DeclRefExpr * declexpr = dyn_cast<DeclRefExpr>(left)) {
			   Decl * decl = declexpr->getFoundDecl();
			   mStack.back().bindDecl(decl, val);
		   } else {
			   heap.Update(tempHeapAddr, val);
		   }
		   return;
	    }
		int leftVal = mStack.back().getStmtVal(left, mContext);
		int rightVal = mStack.back().getStmtVal(right, mContext);
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
			//    if (auto constantArrayType = dyn_cast<ConstantArrayType>(vardecl->getType().getTypePtr())) {
			// 		for (int i = 0;i < constantArrayType->getSize().getSExtValue(); i++) {
			// 			mStack.back().bindDecl(vardecl, 0);
			// 		}
			//    } else {
			//    		mStack.back().bindDecl(vardecl, 0);
			//    }
			   int init = 0;
			   if (vardecl->hasInit()) {
				 init = mStack.back().getStmtVal(vardecl->getInit(), mContext);
			   }
			   mStack.back().bindDecl(vardecl, init);
		   }
		   
	   }
   }
   
   void declref(DeclRefExpr * declref) {
	   mStack.back().setPC(declref);
	   if (declref->getType()->isIntegerType() ||
            declref->getType()->isPointerType() && !declref->getType()->isFunctionPointerType() ||
            declref->getType()->isArrayType()) {
		   Decl* decl = declref->getFoundDecl();
		   int val;
		   if (!getDecl(decl, val)) {
			assert("GetDecl Failed!\n");
		   }
		   mStack.back().bindStmt(declref, val);
	   }
   }

   void cast(CastExpr * castexpr) {
	   mStack.back().setPC(castexpr);
	   if (castexpr->getType()->isIntegerType() ||
            castexpr->getType()->isPointerType() && !castexpr->getType()->isFunctionPointerType() ||
            castexpr->getType()->isArrayType()) {
		   Expr * expr = castexpr->getSubExpr();
		   int val = mStack.back().getStmtVal(expr, mContext);
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
		   val = mStack.back().getStmtVal(decl, mContext);
		   llvm::errs() << val;
	   } else if (callee == mMalloc) {
		   Expr * expr = callexpr->getArg(0);
		   val = mStack.back().getStmtVal(expr, mContext);
		   int addr = heap.Malloc(val);
		   mStack.back().bindStmt(callexpr, addr);
	   } else if (callee == mFree) {
		   Expr* expr = callexpr->getArg(0);
		   int addr = mStack.back().getStmtVal(expr, mContext);
		   heap.Free(addr);
		   mStack.back().bindStmt(callexpr, addr);
	   } else {
		   /// You could add your code here for Function call Return
		   StackFrame newFrame;
		   for (int i = 0;i < callexpr->getNumArgs(); i++) {
			    int val = mStack.back().getStmtVal(
					callexpr->getArg(i), mContext);
				newFrame.bindDecl(callee->getParamDecl(i), val);
           }
		   mStack.push_back(newFrame);
		   visitor->VisitStmt(callee->getBody());
		   int retVal = mStack.back().getRetVal();
		   mStack.pop_back();
		   mStack.back().bindStmt(callexpr, retVal);
		   
	   }
   }
   void ret(ReturnStmt* returnStmt) {
	   Expr* expr = returnStmt->getRetValue();
	   int retVal = mStack.back().getStmtVal(expr, mContext);
	   mStack.back().setRetVal(retVal);
   }
   void unary(UnaryOperator* unaryOperator) {
	   mStack.back().setPC(unaryOperator);
	   Expr* subExpr = unaryOperator->getSubExpr();
	   int val;
	   switch (unaryOperator->getOpcode()) {
			default:
				llvm::errs() << "Unsupported Unary Opcode!\n";
				break;
			case clang::UO_Deref:
				tempHeapAddr = mStack.back().getStmtVal(subExpr, mContext);
				val = heap.Get(tempHeapAddr);
				break;
			case clang::UO_Minus:
				val = -mStack.back().getStmtVal(subExpr, mContext);
				break;
	   }
	   mStack.back().bindStmt(unaryOperator, val);
	   return;
   }

   bool getCond(Stmt* cond) {
	  mStack.back().setPC(cond);
	  return mStack.back().getStmtVal(cond, mContext) != 0;
   }
   
   void unaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *unaryExprOrTypeTraitExpr) {
	  mStack.back().setPC(unaryExprOrTypeTraitExpr);
	//   llvm::errs() << unaryExprOrTypeTraitExpr->get << "\n";
	  mStack.back().bindStmt(unaryExprOrTypeTraitExpr, sizeof(int));
   }

   void parenExpr(ParenExpr *parenExpr) {
        Expr *subExpr = parenExpr->getSubExpr();
        int val = mStack.back().getStmtVal(subExpr, mContext);
        mStack.back().bindStmt(parenExpr, val);
    }
//    void arraySubscriptExpr(ArraySubscriptExpr* arraySubscriptExpr) {
// 	  mStack.back().setPC(arraySubscriptExpr);
// 	  int idx = mStack.back().getStmtVal(arraySubscriptExpr->getIdx(), mContext);
	  
//    }
};


#endif