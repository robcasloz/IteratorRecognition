//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/ShadowDependenceGraph.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVectorImpl

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

#include "llvm/ADT/SetVector.h"
// using llvm::SetVector

#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/Debug.h"
// using llvm::dbgs

#include <iterator>
// using std::back_inserter

namespace iteratorrecognition {

// bool ArePointersIteratorDependent(const llvm::Value *V1,
// const llvm::Value *V2) {
// if (llvm::isa<llvm::Argument>(V1) || llvm::isa<llvm::Argument>(V2)) {
// llvm::report_fatal_error("Pointers are function arguments!");
//}

// auto *ptrConst1 = llvm::dyn_cast<llvm::AllocaInst>(V1);
// auto *ptrConst2 = llvm::dyn_cast<llvm::AllocaInst>(V2);

// if (ptrConst1 && ptrConst2 && ptrConst1 != ptrConst2) {
// return false;
//}

// auto *ptrInst1 = llvm::dyn_cast<llvm::Instruction>(V1);
// auto *ptrInst2 = llvm::dyn_cast<llvm::Instruction>(V2);

// if (!ptrInst1 || !ptrInst2) {
// return false;
//}

// if (llvm::isa<llvm::AllocaInst>(ptrInst1) &&
// llvm::isa<llvm::AllocaInst>(ptrInst2) && ptrInst1 != ptrInst2) {
// return false;
//}
//}

template <typename GraphT>
void DetectOperationsOn(llvm::Instruction *Target,
                        const SDependenceGraph<GraphT> &SDG,
                        llvm::SmallVectorImpl<llvm::Value *> &Operations) {
  llvm::SetVector<llvm::Value *> workList;
  llvm::SmallPtrSet<llvm::Value *, 32> visited, operations;

  workList.insert(Target);

  while (!workList.empty()) {
    auto *curTarget =
        llvm::dyn_cast<llvm::Instruction>(workList.pop_back_val());

    if (!curTarget || visited.count(curTarget)) {
      continue;
    }
    visited.insert(curTarget);

    const auto *curTargetNode = SDG.findNodeFor(curTarget);
    if (!curTargetNode) {
      continue;
    }

    llvm::SmallPtrSet<llvm::Value *, 16> inverse_edge_units;
    for (auto *ie : curTargetNode->inverse_edges()) {
      for (auto *iu : ie->units()) {
        inverse_edge_units.insert(iu);
      }
    }

    operations.insert(curTarget);

    if (auto *ii = llvm::dyn_cast<llvm::StoreInst>(curTarget)) {
      // TODO maybe this should be the next target
      if (inverse_edge_units.count(ii->getValueOperand())) {
        workList.insert(ii->getValueOperand());
      }
    } else if (auto *ii = llvm::dyn_cast<llvm::LoadInst>(curTarget)) {
      // TODO maybe this should be the next target
    } else if (auto *ii = llvm::dyn_cast<llvm::BinaryOperator>(curTarget)) {
      for (auto &op : ii->operands()) {
        if (inverse_edge_units.count(op.get())) {
          workList.insert(op);
        }
      }
    } else if (auto *ii = llvm::dyn_cast<llvm::CmpInst>(curTarget)) {
      for (auto &op : ii->operands()) {
        if (inverse_edge_units.count(op.get())) {
          workList.insert(op);
        }
      }
    } else if (auto *ii = llvm::dyn_cast<llvm::SelectInst>(curTarget)) {
      if (inverse_edge_units.count(ii->getTrueValue())) {
        workList.insert(ii->getTrueValue());
      }
      if (inverse_edge_units.count(ii->getFalseValue())) {
        workList.insert(ii->getFalseValue());
      }
      // do not insert as operation
      // skip condition
      // just aggregate the operands to unify the cfg
      operations.erase(curTarget);
    } else if (auto *ii = llvm::dyn_cast<llvm::GetElementPtrInst>(curTarget)) {
      for (auto &op : ii->operands()) {
        if (inverse_edge_units.count(op.get())) {
          workList.insert(op);
        }
      }
    } else if (auto *ii = llvm::dyn_cast<llvm::PHINode>(curTarget)) {
      for (auto &op : ii->incoming_values()) {
        if (inverse_edge_units.count(op)) {
          workList.insert(op);
        }
      }
    } else {
      llvm::dbgs() << "unhandled instruction: " << *curTarget << '\n';
      // TODO see what to do with this
      operations.erase(curTarget);
      break;
    }
  } // work list iteration end

  std::copy(operations.begin(), operations.end(),
            std::back_inserter(Operations));
}

} // namespace iteratorrecognition
