#include "Dataflow.h"
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
    if (auto loadInst = dyn_cast<LoadInst>(inst)) {
      dumpInst(inst);
      auto src = loadInst->getOperand(0);
      if (dfval->ptrInfos.find(src) != dfval->ptrInfos.end()) {
        dfval->ptrInfos[inst] = dfval->ptrInfos[src];
        // errs() << *dfval;
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
        // errs() << *dfval;
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
        // errs() << *dfval;
      }
    }
  }
};

class PointerAnalysisPass : public ModulePass {
public:
  static char ID;
  PointerAnalysisPass() : ModulePass(ID) {}
  bool runOnModule(Module &M) override {
    for (auto it = M.begin(); it != M.end(); it++) {
      PointerAnalysisVisitor visitor;
      DataflowResult<PointerAnalysisInfo>::Type result;
      PointerAnalysisInfo initval;
#ifdef DEBUG_INFO
      errs() << "Function: " << it->getName() << "\n";
#endif
      compForwardDataflow(&*it, &visitor, &result, initval);
      std::map<Value *, std::set<Function *>> res;
      for (auto i : result) {
        auto ptrinfos = i.second.second.ptrInfos;
        for (auto ptrinfo : ptrinfos) {
          auto inst = ptrinfo.first;
          for (auto function : ptrinfo.second) {
            res[inst].insert(function);
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
    }
    return false;
  }
};