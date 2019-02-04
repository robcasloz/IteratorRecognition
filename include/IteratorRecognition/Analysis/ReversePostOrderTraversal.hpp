//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo

#include "llvm/Analysis/LoopIterator.h"
// using llvm::LoopBlocksRPO

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl

namespace iteratorrecognition {

class ModRefReversePostOrder {
  llvm::SmallVector<llvm::Instruction *, 8> order;

public:
  ModRefReversePostOrder(
      const llvm::Loop &CurLoop, const llvm::LoopInfo &LI,
      const llvm::SmallPtrSetImpl<llvm::Instruction *> &PartOf) {
    llvm::LoopBlocksRPO rpot(const_cast<llvm::Loop *>(&CurLoop));
    rpot.perform(const_cast<llvm::LoopInfo *>(&LI));

    for (auto *bb : rpot) {
      for (auto &ii : *bb) {
        if (ii.mayReadOrWriteMemory() && PartOf.count(&ii)) {
          order.push_back(&ii);
        }
      }
    }
  }

  using iterator = decltype(order)::iterator;
  using const_iterator = decltype(order)::const_iterator;

  decltype(auto) begin() { return order.begin(); }
  decltype(auto) end() { return order.end(); }

  decltype(auto) begin() const { return order.begin(); }
  decltype(auto) end() const { return order.end(); }
};

} // namespace iteratorrecognition
