#include "Dataflow.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <map>
#include <memory>
#include <set>
using namespace llvm;
using ValueSet = std::set<Value *>;
using FunctionSet = std::set<Function *>;
template <class T> using ValueMap = std::map<Value *, T>;
template <class T> using FunctionMap = std::map<Function *, T>;
struct PointerAnalysisInfo {
  ValueMap<ValueSet> ptrInfos;
  ValueMap<ValueSet> objInfos;
  PointerAnalysisInfo() : ptrInfos() {}
  PointerAnalysisInfo(const PointerAnalysisInfo &info)
      : ptrInfos(info.ptrInfos), objInfos(info.objInfos) {}
  bool operator==(const PointerAnalysisInfo &info) const {
    return ptrInfos == info.ptrInfos && objInfos == info.objInfos;
  }
};

inline raw_ostream &operator<<(raw_ostream &out,
                               const PointerAnalysisInfo &info) {
  return out;
}
static void dumpInst(Instruction *inst) {
#ifdef DEBUG_INFO
  inst->dump();
#endif
}
static void dumpInfo(Value *value, ValueSet valueSet) {
#ifdef DEBUG_INFO
  errs() << value->getName() << ": ";
  bool first = true;
  for (auto v : valueSet) {
    if (first) {
      first = false;
    } else {
      errs() << ", ";
    }
    errs() << v->getName();
  }
  errs() << "\n";
#endif
}
static ValueSet resolveObjInfo(Value *value, ValueMap<ValueSet> &valueMap) {
  ValueSet ans;
  ValueSet vWorkList{value};
  while (!vWorkList.empty()) {
    auto v = *vWorkList.begin();
    vWorkList.erase(vWorkList.begin());
    if (valueMap.find(v) != valueMap.end()) {
      for (auto v_add : valueMap[v]) {
        vWorkList.insert(v_add);
      }
    } else {
      ans.insert(v);
    }
  }
  return ans;
}
static FunctionMap<DataflowResult<PointerAnalysisInfo>::Type> results;
static FunctionSet worklist;
static FunctionMap<ValueSet> returns;
static FunctionSet allFunctions;
static FunctionMap<bool> needReturn;
class PointerAnalysisVisitor
    : public DataflowVisitor<struct PointerAnalysisInfo> {
public:
  PointerAnalysisVisitor() {}
  void merge(PointerAnalysisInfo *dest,
             const PointerAnalysisInfo &src) override {
    for (auto src_info : src.ptrInfos) {
      auto v = src_info.first;
      auto valueSet = src_info.second;
      for (auto value : valueSet) {
        dest->ptrInfos[v].insert(value);
      }
    }
    for (auto src_info : src.objInfos) {
      auto v = src_info.first;
      auto valueSet = src_info.second;
      for (auto value : valueSet) {
        dest->objInfos[v].insert(value);
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
    if (needReturn[inst->getFunction()]) {
      return;
    }
    if (auto storeInst = dyn_cast<StoreInst>(inst)) {
      dumpInst(inst);
      auto src = storeInst->getOperand(0);
      auto dstValue = storeInst->getOperand(1);
      for (auto dst : resolveObjInfo(dstValue, dfval->objInfos)) {
        dfval->ptrInfos[dst] = resolveObjInfo(src, dfval->objInfos);
        dumpInfo(dst, dfval->ptrInfos[dst]);
      }
    }
    if (auto loadInst = dyn_cast<LoadInst>(inst)) {
      dumpInst(inst);
      auto srcValue = loadInst->getOperand(0);
      auto dst = loadInst;
      ValueSet loadSet;
      for (auto src : resolveObjInfo(srcValue, dfval->objInfos)) {
        for (auto value : dfval->ptrInfos[src]) {
          loadSet.insert(value);
        }
      }
      dfval->objInfos[dst] = loadSet;
      dumpInfo(dst, loadSet);
    }
    if (auto callInst = dyn_cast<CallInst>(inst)) {
      dumpInst(inst);
      // get Possible CalledFunctions
      auto calledFunctions =
          resolveObjInfo(callInst->getCalledValue(), dfval->objInfos);
      dumpInfo(callInst->getCalledValue(), calledFunctions);
      // process Args
      for (int i = 0; i < callInst->getNumArgOperands(); i++) {
        auto argi = callInst->getOperand(i);
        auto argiType = argi->getType();
        if (!argiType->isPointerTy()) {
          continue;
        }
        // get Possible argi Objects
        auto argiValues = resolveObjInfo(argi, dfval->objInfos);
        dumpInfo(argi, argiValues);
        for (auto calledFunction : calledFunctions) {
          auto called = dyn_cast<Function>(calledFunction);
          if (!called)
            continue;
          auto parami = called->getArg(i);
          auto objInfos = results[called][&*called->begin()].first.objInfos;
          auto ptrInfos = results[called][&*called->begin()].first.ptrInfos;
          // pass ptrInfo and objInfo to CalledFunction
          for (auto value : dfval->objInfos) {
            auto keyValue = value.first;
            auto valueSet = value.second;
            for (auto v : valueSet) {
              results[called][&*called->begin()]
                  .first.objInfos[keyValue]
                  .insert(v);
            }
          }
          for (auto value : dfval->ptrInfos) {
            auto keyValue = value.first;
            auto valueSet = value.second;
            for (auto v : valueSet) {
              results[called][&*called->begin()]
                  .first.ptrInfos[keyValue]
                  .insert(v);
            }
          }
          // pass ArgValues
          for (auto argiValue : argiValues) {
            results[called][&*called->begin()].first.objInfos[parami].insert(
                argiValue);
          }
          if (!(objInfos ==
                results[called][&*called->begin()].first.objInfos) ||
              !(ptrInfos ==
                results[called][&*called->begin()].first.ptrInfos)) {
            worklist.insert(called);
            needReturn[callInst->getFunction()] = true;
            worklist.insert(callInst->getFunction());
          }
        }
      }
      // pass return ptrInfo
      ValueMap<ValueSet> changed;
      for (auto calledFunction : calledFunctions) {
        auto called = dyn_cast<Function>(calledFunction);
        if (!called)
          continue;
        for (auto it = called->begin(); it != called->end(); it++) {
          auto bb = &*it;
          if (succ_begin(bb) != succ_end(bb)) {
            continue;
          }
          auto ptrInfos = results[called][bb].second.ptrInfos;
          for (auto ptr : ptrInfos) {
            auto keyValue = ptr.first;
            for (auto addValue : ptr.second) {
              changed[keyValue].insert(addValue);
              dfval->ptrInfos[keyValue].insert(addValue);
#ifdef DEBUG_INFO
              errs() << keyValue->getName() << " add: " << addValue->getName()
                     << "\n";
#endif
            }
          }
        }
      }
      for (auto pair : changed) {
        dfval->ptrInfos[pair.first] = pair.second;
      }
      // process return values
      auto dst = callInst;
      dfval->objInfos[dst].clear();
      for (auto calledFunction : calledFunctions) {
        auto called = dyn_cast<Function>(calledFunction);
        if (!called)
          continue;
        for (auto value : returns[called]) {
          dfval->objInfos[dst].insert(value);
        }
      }
    }
    if (auto gepInst = dyn_cast<GetElementPtrInst>(inst)) {
      dumpInst(inst);
      auto src = gepInst->getOperand(0);
      auto dst = gepInst;
      dfval->objInfos[dst] = resolveObjInfo(src, dfval->objInfos);
      dumpInfo(dst, dfval->objInfos[dst]);
    }
    if (auto returnInst = dyn_cast<ReturnInst>(inst)) {
      dumpInst(inst);
      auto returnValue = returnInst->getReturnValue();
      auto returnSet = resolveObjInfo(returnValue, dfval->objInfos);
      if (returns[returnInst->getFunction()] != returnSet) {
        returns[returnInst->getFunction()] = returnSet;
        for (auto function : allFunctions) {
          worklist.insert(function);
        }
      }
    }
  }
};

class PointerAnalysisPass : public ModulePass {
public:
  static char ID;
  PointerAnalysisPass() : ModulePass(ID) {}
  bool runOnModule(Module &M) override {
    PointerAnalysisVisitor visitor;

    for (auto it = M.begin(); it != M.end(); it++) {
      worklist.insert(&*it);
      allFunctions.insert(&*it);
      needReturn[&*it] = false;
    }
    while (!worklist.empty()) {
      auto function = *worklist.begin();
      worklist.erase(worklist.begin());
      needReturn[function] = false;
      if (function->empty()) {
        continue;
      }
#ifdef DEBUG_INFO
      errs() << "Process Function: " << function->getName() << "\n";
#endif
      auto bb_begin = &*function->begin();
      compForwardDataflow(function, &visitor, &results[function],
                          results[function][bb_begin].first);
    }

    // print Results
    for (auto &function : M) {
#ifdef DEBUG_INFO
      errs() << "Print Function: " << function.getName() << "\n";
#endif
      for (auto &bb : function) {
        for (auto it = bb.begin(); it != bb.end(); it++) {
          auto inst = &*it;
          auto callInst = dyn_cast<CallInst>(inst);
          if (!callInst || isa<DbgInfoIntrinsic>(inst) ||
              isa<MemIntrinsic>(inst)) {
            continue;
          }
          dumpInst(inst);
          auto calledValue = callInst->getCalledValue();
          auto objInfos = results[&function][&bb].second.objInfos;
          auto calledFunctions = resolveObjInfo(calledValue, objInfos);
          dumpInfo(calledValue, calledFunctions);
          errs() << callInst->getDebugLoc().getLine() << " :";
          bool first = true;
          for (auto calledFunction : calledFunctions) {
            if (!calledFunction->hasName())
              continue;
            auto called = dyn_cast<Function>(calledFunction);
            if (!called)
              continue;
            if (first) {
              first = false;
              errs() << " ";
            } else {
              errs() << ", ";
            }
            errs() << called->getName();
          }
          errs() << "\n";
        }
      }
    }
    return false;
  }
};