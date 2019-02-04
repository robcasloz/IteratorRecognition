//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::Argument
// using llvm::GetElementPtrInst
// using llvm::LoadInst
// using llvm::StoreInst
// using llvm::SelectInst

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVectorImpl
// using llvm::SmallVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl
// using llvm::SmallPtrSet

#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/Debug.h"
// using llvm::dbgs

namespace iteratorrecognition {

enum class IteratorVarianceValue { Unknown, Invariant, Variant };

class IteratorVariance {
  IteratorVarianceValue Val;

public:
  explicit IteratorVariance(
      IteratorVarianceValue V = IteratorVarianceValue::Unknown)
      : Val(V) {}

  decltype(auto) get() const { return Val; }

  bool operator==(const IteratorVarianceValue &RhsVal) const {
    return this->Val == RhsVal;
  }
  bool operator==(const IteratorVariance &Rhs) const { return this->Val == Rhs.Val; }

  bool mergeIn(const IteratorVariance &Other) { return mergeIn(Other.Val); }

  bool mergeIn(const IteratorVarianceValue &OtherVal) {
    auto curVal = Val;

    switch (curVal) {
    case IteratorVarianceValue::Unknown:
      curVal = OtherVal;
      break;
    case IteratorVarianceValue::Invariant:
    case IteratorVarianceValue::Variant:
    default:
      if (OtherVal == IteratorVarianceValue::Variant) {
        curVal = IteratorVarianceValue::Variant;
      }

      break;
    };

    bool hasChanged = (curVal == Val);
    Val = curVal;

    return hasChanged;
  }
};

// TODO this might need a cache

IteratorVariance
GetIteratorVariance(const llvm::Value *V,
                    const llvm::SmallPtrSetImpl<llvm::Instruction *> &Iterators,
                    const llvm::Loop *CurLoop = nullptr) {
  llvm::SmallVector<const llvm::Value *, 8> workList{V};
  llvm::SmallPtrSet<const llvm::Value *, 8> visited;

  IteratorVariance status{};

  while (!workList.empty()) {
    if (status == IteratorVarianceValue::Variant) {
      break;
    }

    auto *V = workList.pop_back_val();

    if (visited.count(V)) {
      continue;
    }

    if (llvm::isa<llvm::Constant>(V) || llvm::isa<llvm::Argument>(V)) {
      status.mergeIn(IteratorVarianceValue::Invariant);
      continue;
    }

    auto *I = llvm::dyn_cast<llvm::Instruction>(V);
    if (!I) {
      llvm::dbgs() << "Unhandled value: " << *V << '\n';
    }

    if (Iterators.count(I)) {
      status.mergeIn(IteratorVarianceValue::Variant);
      continue;
    }

    if (CurLoop && !CurLoop->contains(I)) {
      status.mergeIn(IteratorVarianceValue::Invariant);
      continue;
    }

    if (auto *gepI = llvm::dyn_cast<llvm::GetElementPtrInst>(I)) {
      workList.push_back(gepI->getPointerOperand());
      for (auto &idx : gepI->indices()) {
        workList.push_back(idx.get());
      }
    } else if (auto *loadI = llvm::dyn_cast<llvm::LoadInst>(I)) {
      workList.push_back(loadI->getPointerOperand());
    } else if (auto *storeI = llvm::dyn_cast<llvm::StoreInst>(I)) {
      workList.push_back(storeI->getPointerOperand());
    } else if (auto *selectI = llvm::dyn_cast<llvm::SelectInst>(I)) {
      workList.push_back(selectI->getOperand(1));
      workList.push_back(selectI->getOperand(2));
    } else {
      llvm::dbgs() << "Unhandled instruction: " << *I << '\n';
    }
  }

  return status;
}

} // namespace iteratorrecognition
