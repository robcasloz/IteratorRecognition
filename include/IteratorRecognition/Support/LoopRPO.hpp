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

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

namespace llvm {
class Instruction;
class BasicBlock;
} // namespace llvm

namespace iteratorrecognition {

class LoopRPO {
  template <typename T> using OrderTy = llvm::SmallVector<T, 64>;
  using InstOrderTy = OrderTy<llvm::Instruction *>;

  InstOrderTy Order;

public:
  LoopRPO() = default;
  LoopRPO(const LoopRPO &) = default;
  LoopRPO &operator=(const LoopRPO &) = default;

  template <typename PredT>
  LoopRPO(const llvm::Loop &CurLoop, const llvm::LoopInfo &LI,
          PredT Pred = [](const auto &e) { return true; }) {
    llvm::LoopBlocksRPO rpot(const_cast<llvm::Loop *>(&CurLoop));
    rpot.perform(const_cast<llvm::LoopInfo *>(&LI));

    for (auto *bb : rpot) {
      for (auto &ii : *bb) {
        if (Pred(ii)) {
          Order.push_back(&ii);
        }
      }
    }
  }

  using iterator = InstOrderTy::iterator;
  using const_iterator = InstOrderTy::const_iterator;

  decltype(auto) begin() { return Order.begin(); }
  decltype(auto) end() { return Order.end(); }

  decltype(auto) begin() const { return Order.begin(); }
  decltype(auto) end() const { return Order.end(); }
};

} // namespace iteratorrecognition
