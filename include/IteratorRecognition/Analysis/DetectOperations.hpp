//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/ShadowDependenceGraph.hpp"

#include "llvm/IR/Instructions.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVectorImpl
// using llvm::SmallVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl
// using llvm::SmallPtrSet

namespace iteratorrecognition {

template <typename GraphT>
void DetectOperationsOn(
    llvm::Instruction *Target, const SDependenceGraph<GraphT> &SDG,
    llvm::SmallVectorImpl<llvm::Instruction *> &Operations) {
  llvm::SmallVector<llvm::Instruction *, 8> workList;
  llvm::SmallPtrSet<llvm::Instruction *, 8> visited;

  workList.push_back(Target);

  while (!workList.empty()) {
    auto *curTarget = workList.back();
    workList.pop_back();

    const auto *curTargetNode = SDG.findNodeFor(curTarget);

    if (!curTargetNode) {
      continue;
    }

    if (visited.count(curTarget)) {
      continue;
    }
    visited.insert(curTarget);

    for (auto *ie : curTargetNode->inverse_edges()) {
      for (auto *iu : ie->units()) {
        if (llvm::isa<llvm::BinaryOperator>(iu)) {
          for (unsigned i = 0; i < iu->getNumOperands(); ++i) {
            llvm::Instruction *op =
                llvm::dyn_cast<llvm::Instruction>(iu->getOperand(i));

            if (op) {
              workList.push_back(op);
            }
          }
          llvm::dbgs() << "skata\n";

          Operations.emplace_back(iu);
        } else if (llvm::isa<llvm::StoreInst>(iu)) {
          llvm::Instruction *op =
              llvm::dyn_cast<llvm::Instruction>(iu->getOperand(0));

          if (op) {
            workList.push_back(op);
          }
        } else if (llvm::isa<llvm::CmpInst>(iu)) {
          for (unsigned i = 0; i < iu->getNumOperands(); ++i) {
            llvm::Instruction *op =
                llvm::dyn_cast<llvm::Instruction>(iu->getOperand(i));

            if (op) {
              workList.push_back(op);
            }
          }

          Operations.emplace_back(iu);
        } else if (llvm::isa<llvm::LoadInst>(iu)) {
          for (unsigned i = 0; i < iu->getNumOperands(); ++i) {
            llvm::Instruction *op =
                llvm::dyn_cast<llvm::Instruction>(iu->getOperand(i));

            if (op) {
              workList.push_back(op);
            }
          }
        } else if (llvm::isa<llvm::SelectInst>(iu)) {
          for (unsigned i = 1; i < iu->getNumOperands(); ++i) {
            llvm::Instruction *op =
                llvm::dyn_cast<llvm::Instruction>(iu->getOperand(i));

            if (op) {
              workList.push_back(op);
            }
          }
        } else if (llvm::isa<llvm::GetElementPtrInst>(iu)) {
          for (unsigned i = 0; i < iu->getNumOperands(); ++i) {
            llvm::Instruction *op =
                llvm::dyn_cast<llvm::Instruction>(iu->getOperand(i));

            if (op) {
              workList.push_back(op);
            }
          }
        } else {
          llvm::dbgs() << "unhandled instruction: " << *iu << '\n';
          Operations.clear();
          break;
        }
      }
    }
  }
}

} // namespace iteratorrecognition
