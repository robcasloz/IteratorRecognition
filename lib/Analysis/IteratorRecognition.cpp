//
//
//

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "llvm/IR/BasicBlock.h"
// using llvm::BasicBlock

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

namespace iteratorrecognition {

bool HasPayloadOnlyBlocks(const IteratorInfo &Info) {
  llvm::SmallPtrSet<llvm::BasicBlock *, 16> itBlocks;

  for (auto *e : Info) {
    itBlocks.insert(e->getParent());
  }

  for (auto *e : Info.getLoop()->getBlocks()) {
    if (!itBlocks.count(e)) {
      return true;
    }
  }

  return false;
}

void GetPayloadOnlyBlocks(const IteratorInfo &Info,
                          llvm::SmallVectorImpl<llvm::BasicBlock *> Blocks) {
  llvm::SmallPtrSet<llvm::BasicBlock *, 16> itBlocks;

  for (auto *e : Info) {
    itBlocks.insert(e->getParent());
  }

  for (auto *e : Info.getLoop()->getBlocks()) {
    if (!itBlocks.count(e)) {
      Blocks.push_back(e);
    }
  }
}

} // namespace iteratorrecognition
