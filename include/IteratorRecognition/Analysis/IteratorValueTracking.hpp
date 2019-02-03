//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::Argument
// using llvm::AllocaInst

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVectorImpl
// using llvm::SmallVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl
// using llvm::SmallPtrSet

#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/Debug.h"
// using llvm::dbgs

//#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

namespace iteratorrecognition {

enum class IteratorQueryResult { Unknown, IteratorInvariant, IteratorVariant };

// TODO this might need a cache

IteratorQueryResult
GetIteratorDependent(llvm::Value *V,
                     llvm::SmallPtrSetImpl<llvm::Instruction *> &Iterators) {
  llvm::SmallVector<llvm::Value *, 8> workList{V};
  llvm::SmallPtrSet<llvm::Value *, 8> visited;
  auto status = IteratorQueryResult::Unknown;

  while (!workList.empty()) {
    auto *V = workList.pop_back_val();
    llvm::dbgs() << "checking: " << *V << '\n';

    if (visited.count(V)) {
      continue;
    }

    if (llvm::isa<llvm::Constant>(V) || llvm::isa<llvm::Argument>(V)) {
      return IteratorQueryResult::IteratorInvariant;
    }

    auto *I = llvm::dyn_cast<llvm::Instruction>(V);

    if (Iterators.count(I)) {
      return IteratorQueryResult::IteratorVariant;
    }

    switch (I->getOpcode()) {
    case llvm::Instruction::GetElementPtr: {
      // TODO handle indices
      auto *gepI = llvm::dyn_cast<llvm::GetElementPtrInst>(I);
      workList.push_back(gepI->getPointerOperand());
    } break;
    case llvm::Instruction::Load: {
      auto *loadI = llvm::dyn_cast<llvm::LoadInst>(I);
      workList.push_back(loadI->getPointerOperand());
    } break;
    case llvm::Instruction::Store: {
      auto *storeI = llvm::dyn_cast<llvm::StoreInst>(I);
      workList.push_back(storeI->getPointerOperand());
    } break;
    case llvm::Instruction::Select: {
      auto *selectI = llvm::dyn_cast<llvm::SelectInst>(I);
      workList.push_back(selectI->getOperand(1));
      workList.push_back(selectI->getOperand(2));
    } break;
    default:
      llvm::dbgs() << "unhandled instruction: " << *I << '\n';
    };
  }

  return status;
}

} // namespace iteratorrecognition
