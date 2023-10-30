//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;
static ManagedStatic<LLVMContext> GlobalContext;
static LLVMContext &getGlobalContext() { return *GlobalContext; }
/* In LLVM 5.0, when  -O0 passed to clang , the functions generated with clang
 * will have optnone attribute which would lead to some transform passes
 * disabled, like mem2reg.
 */

struct EnableFunctionOptPass : public FunctionPass {
  static char ID;
  EnableFunctionOptPass() : FunctionPass(ID) {}
  bool runOnFunction(Function &F) override {
    if (F.hasFnAttribute(Attribute::OptimizeNone)) {
      F.removeFnAttr(Attribute::OptimizeNone);
    }
    return true;
  }
};

char EnableFunctionOptPass::ID = 0;

///!TODO TO BE COMPLETED BY YOU FOR ASSIGNMENT 2
/// Updated 11/10/2017 by fargo: make all functions
/// processed by mem2reg before this pass.
struct FuncPtrPass : public ModulePass {
  static char ID;  // Pass identification, replacement for typeid
  FuncPtrPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override {
    // errs() << "Hello: ";
    // errs().write_escaped(M.getName()) << '\n';
    // M.dump();
    // errs() << "------------------------------\n";
    std::map<Instruction *, std::set<Function *>> result;
    std::map<Value *, std::set<Function *>> possibleFuncs;
    std::vector<Function *> funcWorkList;
    for (Function &F : M) {
      funcWorkList.push_back(&F);
    }
    while (!funcWorkList.empty()) {
      Function *F = funcWorkList.back();
      funcWorkList.pop_back();
      for (auto BIt = F->begin(); BIt != F->end(); BIt++) {
        for (auto IIt = BIt->begin(); IIt != BIt->end(); IIt++) {
          // find function based on
          //  Function itself/phiInst/arg_possibleFuncs/ReturnValue
          auto find = [&](Value *value) -> std::set<Function *> {
            std::set<Function *> funcs;
            std::vector<Value *> workList;
            workList.push_back(value);
            while (!workList.empty()) {
              Value *value = workList.back();
              // errs() << value << "\n";
              workList.pop_back();
              if (Function *func = dyn_cast<Function>(value)) {
                funcs.insert(func);
              } else if (Instruction *instr = dyn_cast<Instruction>(value)) {
                if (instr->getOpcode() == Instruction::PHI) {
                  for (auto i = 0; i < instr->getNumOperands(); i++) {
                    workList.push_back(instr->getOperand(i));
                  }
                } else if (CallBase *call = dyn_cast<CallBase>(instr)) {
                  for (auto func : result[call]) {
                    for (auto add : possibleFuncs[func]) {
                      funcs.insert(add);
                    }
                  }
                }
              } else if (isa<Argument>(value)) {
                for (auto func : possibleFuncs[value]) {
                  funcs.insert(func);
                }
              }
            }
            return funcs;
          };
          CallBase *callInst = dyn_cast<CallBase>(&*IIt);
          if (callInst != nullptr) {
            auto calledFunc = callInst->getCalledOperand();
            if (calledFunc->getName() == "llvm.dbg.value") continue;
            // errs() << "Function: " << calledFunc->getName() << "\n";

            // update result
            std::set<Function *> called = find(calledFunc);
            for (auto func : called) {
              result[callInst].insert(func);
            }
            // update args
            for (int i = 0; i < callInst->getNumArgOperands(); i++) {
              Value *arg = callInst->getArgOperand(i);
              if (arg->getType()->isPointerTy() &&
                  arg->getType()->getPointerElementType()->isFunctionTy()) {
                std::set<Function *> passed = find(arg);

                for (auto func : result[callInst]) {
                  int cnt = possibleFuncs[func->getArg(i)].size();
                  for (auto addFunc : passed) {
                    possibleFuncs[func->getArg(i)].insert(addFunc);
                  }
                  if (cnt < possibleFuncs[func->getArg(i)].size()) {
                    funcWorkList.push_back(func);
                  }
                }
              }
            }

          } else if (IIt->getOpcode() == Instruction::Ret) {  // update return
            // errs() << "Function: " << F->getName() << "\n";
            auto funcs = find(IIt->getOperand(0));
            int cnt = possibleFuncs[F].size();
            for (auto func : funcs) {
              possibleFuncs[F].insert(func);
            }
            if (cnt < possibleFuncs[F].size()) {
              for (Function &F : M) {
                funcWorkList.push_back(&F);
              }
            }
          }
        }
      }
    }
    // Print Results
    std::map<int, std::set<Function *>> finalRes;
    for (auto item : result) {
      finalRes[item.first->getDebugLoc().getLine()] = item.second;
    }
    for (auto item : finalRes) {
      auto line = item.first;
      auto fs = item.second;
      assert(fs.size() > 0);
      auto it = fs.begin();
      errs() << line << " : " << (*it)->getName();
      it++;
      for (; it != fs.end(); it++) {
        errs() << ", " << (*it)->getName();
      }
      errs() << "\n";
    }
    return false;
  }
};

char FuncPtrPass::ID = 0;
static RegisterPass<FuncPtrPass> X("funcptrpass",
                                   "Print function call instruction");

static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("<filename>.bc"),
                                          cl::init(""));

int main(int argc, char **argv) {
  LLVMContext &Context = getGlobalContext();
  SMDiagnostic Err;
  // Parse the command line to read the Inputfilename
  cl::ParseCommandLineOptions(
      argc, argv, "FuncPtrPass \n My first LLVM too which does not do much.\n");

  // Load the input module
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  llvm::legacy::PassManager Passes;

  /// Remove functions' optnone attribute in LLVM5.0
  Passes.add(new EnableFunctionOptPass());
  /// Transform it to SSA
  Passes.add(llvm::createPromoteMemoryToRegisterPass());

  /// Your pass to print Function and Call Instructions
  Passes.add(new FuncPtrPass());
  Passes.run(*M.get());
}
