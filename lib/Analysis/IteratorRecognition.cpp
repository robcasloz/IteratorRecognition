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
                          llvm::SmallVectorImpl<llvm::BasicBlock *> &Blocks) {
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

bool HasMixedBlocks(const IteratorInfo &Info) {
  llvm::SmallVector<llvm::BasicBlock *, 32> blocks;

  GetPayloadOnlyBlocks(Info, blocks);

  for (auto *e : Info.getLoop()->getBlocks()) {
    auto found = std::find(blocks.begin(), blocks.end(), e);
    if (found == blocks.end()) {
      continue;
    }

    for (auto &i : *e) {
      if (!Info.isIterator(&i)) {
        return true;
      }
    }
  }

  return false;
}

void GetMixedBlocks(const IteratorInfo &Info,
                    llvm::SmallVectorImpl<llvm::BasicBlock *> &Mixed) {
  llvm::SmallVector<llvm::BasicBlock *, 32> blocks;

  GetPayloadOnlyBlocks(Info, blocks);

  for (auto *e : Info.getLoop()->getBlocks()) {
    auto found = std::find(blocks.begin(), blocks.end(), e);
    if (found == blocks.end()) {
      continue;
    }

    for (auto &i : *e) {
      if (!Info.isIterator(&i)) {
        Mixed.push_back(i.getParent());
        break;
      }
    }
  }
}

bool HasPayloadOnlySubloops(const IteratorInfo &Info) {
  llvm::SmallVector<llvm::BasicBlock *, 32> blocks;

  GetPayloadOnlyBlocks(Info, blocks);

  for (auto *e : Info.getLoop()->getSubLoops()) {
    unsigned blockCount = 0;

    for (auto *b : e->getBlocks()) {
      auto found = std::find(blocks.begin(), blocks.end(), b);
      if (found == blocks.end()) {
        break;
      }

      ++blockCount;
    }

    if (blockCount == e->getNumBlocks()) {
      return true;
    }
  }

  return false;
}

void GetPayloadOnlySubloops(const IteratorInfo &Info,
                            llvm::SmallVectorImpl<llvm::Loop *> &SubLoops) {
  llvm::SmallVector<llvm::BasicBlock *, 32> blocks;

  GetPayloadOnlyBlocks(Info, blocks);

  for (auto *e : Info.getLoop()->getSubLoops()) {
    unsigned blockCount = 0;

    for (auto *b : e->getBlocks()) {
      auto found = std::find(blocks.begin(), blocks.end(), b);
      if (found == blocks.end()) {
        break;
      }

      ++blockCount;
    }

    if (blockCount == e->getNumBlocks()) {
      SubLoops.push_back(e);
    }
  }
}

} // namespace iteratorrecognition
