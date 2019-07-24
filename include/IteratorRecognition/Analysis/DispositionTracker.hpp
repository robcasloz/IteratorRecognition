//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/ValueClassification.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::Argument
// using llvm::GetElementPtrInst
// using llvm::LoadInst
// using llvm::StoreInst
// using llvm::SelectInst

#include "llvm/ADT/SetVector.h"
// using llvm::SetVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

#include <vector>
// using std::vector

#include <cassert>
// using assert

namespace iteratorrecognition {

enum class AccessDisposition { Unknown, Invariant, Variant };

//

class DispositionTracker {
  llvm::Loop *TopL;
  IteratorRecognitionInfo &ITRInfo;
  std::vector<IteratorInfo> Infos;

  bool isIterator(const llvm::Instruction *I, const llvm::Loop *CurL,
                  bool ConsiderSubLoopIterators) const;

  bool init(llvm::Loop *CurL) {
    assert(CurL && "Loop is empty!");
    assert(ITRInfo.getLoopInfo().getLoopFor(CurL->getHeader()) == CurL &&
           "Loop does not belong to this loop info object!");

    auto *topL = CurL;
    while (topL->getParentLoop()) {
      topL = topL->getParentLoop();
    }

    if (TopL == topL) {
      return false;
    }

    reset();
    TopL = topL;

    for (auto *e : const_cast<llvm::LoopInfo &>(ITRInfo.getLoopInfo())
                       .getLoopsInPreorder()) {
      if (TopL->contains(e->getHeader())) {
        auto info = ITRInfo.getIteratorInfoFor(e);
        assert(info.hasValue() && "No iterator information found for loop!");

        Infos.push_back(*info);
      }
    }

    return true;
  }

  void reset() {
    TopL = nullptr;
    Infos.clear();
  }

public:
  DispositionTracker() = delete;

  explicit DispositionTracker(IteratorRecognitionInfo &ITRI)
      : TopL(nullptr), ITRInfo(ITRI) {}

  AccessDisposition getDisposition(const llvm::Value *Query,
                                   const llvm::Loop *L,
                                   bool ConsiderSubLoopIterators = false);
};

} // namespace iteratorrecognition

