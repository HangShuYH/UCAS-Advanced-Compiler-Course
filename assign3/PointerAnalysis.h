#include "Dataflow.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <memory>
#include <set>
using namespace llvm;
struct PointerAnalysisInfo {
  std::map<Value *, std::set<Function *>> ptrInfos;
  PointerAnalysisInfo() : ptrInfos() {}
  PointerAnalysisInfo(const PointerAnalysisInfo &info)
      : ptrInfos(info.ptrInfos) {}
  bool operator==(const PointerAnalysisInfo &info) const {
    return ptrInfos == info.ptrInfos;
  }
};

inline raw_ostream &operator<<(raw_ostream &out,
                               const PointerAnalysisInfo &info) {
  for (auto ii = info.ptrInfos.begin(), ie = info.ptrInfos.end(); ii != ie;
       ++ii) {
    out << ii->first->getName() << ": ";
    bool first = true;
    for (auto func : (*ii).second) {
      if (first) {
        first = false;
      } else {
        out << ", ";
      }
      out << func->getName();
    }
    out << "\n";
  }
  return out;
}
static std::map<Function *, DataflowResult<PointerAnalysisInfo>::Type>
    funcPtrResults;
static std::set<Function *> todoFunctions;
static std::map<Value *, BasicBlock *> structMapping;
static std::map<Value *, Value *> fieldMapping;
static void
addFunction(DataflowResult<PointerAnalysisInfo>::Type &dataflowResult,
            Value *value, Function *function) {
  if (fieldMapping.find(value) != fieldMapping.end()) {
    dataflowResult[structMapping[value]]
        .second.ptrInfos[fieldMapping[value]]
        .insert(function);
  }
}
static std::set<Function *>
getFunction(DataflowResult<PointerAnalysisInfo>::Type &dataflowResult,
            Value *value) {
  if (fieldMapping.find(value) != fieldMapping.end()) {
    return dataflowResult[structMapping[value]]
        .second.ptrInfos[fieldMapping[value]];
  }
  return std::set<Function *>();
}
static void dumpInst(Instruction *inst) {
#ifdef DEBUG_INFO
  inst->dump();
#endif
}
class PointerAnalysisVisitor
    : public DataflowVisitor<struct PointerAnalysisInfo> {
public:
  PointerAnalysisVisitor() {}
  void merge(PointerAnalysisInfo *dest,
             const PointerAnalysisInfo &src) override {
    for (auto src_info : src.ptrInfos) {
      auto inst = src_info.first;
      auto functionSet = src_info.second;
      if (dest->ptrInfos.find(inst) != dest->ptrInfos.end()) {
        for (auto function : functionSet) {
          dest->ptrInfos[inst].insert(function);
        }
      } else {
        dest->ptrInfos[inst] = functionSet;
      }
    }
  }
  void compDFVal(Instruction *inst, PointerAnalysisInfo *dfval) override {
    if (isa<DbgInfoIntrinsic>(inst)) {
      return;
    }
    if (isa<MemIntrinsic>(inst)) {
      return;
    }
    if (auto allocaInst = dyn_cast<AllocaInst>(inst)) {
      auto allocaType = allocaInst->getAllocatedType();
      if (allocaType->isStructTy() || allocaType->isArrayTy()) {
        structMapping[allocaInst->getOperand(0)] = allocaInst->getParent();
      }
    }
    if (auto loadInst = dyn_cast<LoadInst>(inst)) {
      dumpInst(inst);
      auto src = loadInst->getOperand(0);
      if (dfval->ptrInfos.find(src) != dfval->ptrInfos.end()) {
        dfval->ptrInfos[inst] = dfval->ptrInfos[src];
      }
      auto funcs = getFunction(funcPtrResults[inst->getFunction()], src);
      if (!funcs.empty()) {
        dfval->ptrInfos[inst] = funcs;
      }
    }
    if (auto storeInst = dyn_cast<StoreInst>(inst)) {
      dumpInst(inst);
      auto src = storeInst->getOperand(0);
      auto dst = storeInst->getOperand(1);
      std::set<Function *> functionSet;
      if (auto function = dyn_cast<Function>(src)) {
        functionSet.insert(function);
      } else if (dfval->ptrInfos.find(src) != dfval->ptrInfos.end()) {
        functionSet = dfval->ptrInfos[src];
      }
      if (!functionSet.empty()) {
        dfval->ptrInfos[dst] = functionSet;
      }
      for (auto function : functionSet) {
        addFunction(funcPtrResults[inst->getFunction()], dst, function);
      }
    }
    if (auto callInst = dyn_cast<CallInst>(inst)) {
      dumpInst(inst);
      auto src = callInst->getCalledOperand();
      std::set<Function *> functionSet;
      if (auto function = dyn_cast<Function>(src)) {
        functionSet.insert(function);
      } else if (dfval->ptrInfos.find(src) != dfval->ptrInfos.end()) {
        functionSet = dfval->ptrInfos[src];
      }
      if (!functionSet.empty()) {
        dfval->ptrInfos[inst] = functionSet;
      }
      // update args
      for (int i = 0; i < callInst->getNumArgOperands(); i++) {
        auto arg = callInst->getArgOperand(i);
        if (arg->getType()->isPointerTy() &&
            arg->getType()->getPointerElementType()->isFunctionTy() &&
            dfval->ptrInfos.find(arg) != dfval->ptrInfos.end()) {
          auto functions = dfval->ptrInfos[arg];
          for (auto function : dfval->ptrInfos[inst]) {
            auto dataflowResult = &funcPtrResults[function];
            auto bb_begin_info = &(*dataflowResult)[&*function->begin()].first;
            auto ori_info = *bb_begin_info;
            for (auto add_function : functions) {
              bb_begin_info->ptrInfos[function->getArg(i)].insert(add_function);
            }
            if (!(ori_info == *bb_begin_info)) {
              todoFunctions.insert(function);
#ifdef DEBUG_INFO
              errs() << function->getName() << ": "
                     << function->getArg(i)->getName() << "\n";
              for (auto f : functions) {
                errs() << "Arg pass Function: " << f->getName() << "\n";
              }
#endif
            }
          }
        }
      }
    }
    if (auto gepInst = dyn_cast<GetElementPtrInst>(inst)) {
      Value *value = gepInst->getOperand(2);
      fieldMapping[gepInst] = gepInst->getOperand(2);
    }
    if (auto returnInst = dyn_cast<ReturnInst>(inst)) {
    }
  }
};

class PointerAnalysisPass : public ModulePass {
public:
  static char ID;
  PointerAnalysisPass() : ModulePass(ID) {}
  bool runOnModule(Module &M) override {
    todoFunctions.clear();
    fieldMapping.clear();
    structMapping.clear();
    std::set<Function *> funcWorkList;
    for (auto &function : M) {
      funcWorkList.insert(&function);
      funcPtrResults[&function] = DataflowResult<PointerAnalysisInfo>::Type();
    }
    PointerAnalysisVisitor visitor;
    while (!funcWorkList.empty()) {
      auto function = *funcWorkList.begin();
      funcWorkList.erase(funcWorkList.begin());
      if (function->empty()) {
        continue;
      }
#ifdef DEBUG_INFO
      errs() << "Function: " << function->getName() << "\n";
#endif
      auto &bb_begin = *function->begin();
      compForwardDataflow(function, &visitor, &funcPtrResults[function],
                          funcPtrResults[function][&bb_begin].second);
      while (!todoFunctions.empty()) {
        auto todoFuncion = *todoFunctions.begin();
        todoFunctions.erase(todoFunctions.begin());
        funcWorkList.insert(todoFuncion);
      }
    }

    std::map<Value *, std::set<Function *>> res;
    for (auto funcPtrRes : funcPtrResults) {
      for (auto bbResult : funcPtrRes.second) {
        auto ptrInfos = bbResult.second.second.ptrInfos;
        for (auto ptrInfo : ptrInfos) {
          auto inst = ptrInfo.first;
          for (auto function : ptrInfo.second) {
            res[inst].insert(function);
          }
        }
      }
    }
    for (auto info : res) {
      if (auto callInst = dyn_cast<CallInst>(info.first)) {
        errs() << callInst->getDebugLoc().getLine() << " :";
        bool first = true;
        for (auto function : info.second) {
          if (first) {
            first = false;
          } else {
            errs() << ",";
          }
          errs() << " " << function->getName();
        }
        errs() << "\n";
      }
    }
    return false;
  }
};