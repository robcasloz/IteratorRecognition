//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/Analysis/AliasAnalysis.h"
// using llvm::AAResults

#include "llvm/Analysis/MemoryLocation.h"
// using llvm::MemoryLocation

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "IteratorRecognition/Analysis/DetectOperations.hpp"

#include "IteratorRecognition/Analysis/IteratorValueTracking.hpp"

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

namespace iteratorrecognition {

class StaticCommutativityAnalyzer {
public:
  StaticCommutativityAnalyzer() = default;

  void analyze(const llvm::SmallVectorImpl<LoadModifyStore> &LMS,
               const llvm::SmallPtrSetImpl<llvm::Instruction *> &ItVals,
               const llvm::Loop &CurLoop) {

    llvm::dbgs() << "=============\n";
    llvm::dbgs() << "loop: " << CurLoop.getHeader()->getName() << "\n";

    for (auto &e : LMS) {
      llvm::dbgs() << "target: " << *e.Target << '\n';

      llvm::dbgs() << "sources:\n";
      for (auto *s : e.Sources) {
        llvm::dbgs() << *s << '\n';
      }

      llvm::dbgs() << "ops:\n";
      for (auto *o : e.Operations) {
        llvm::dbgs() << *o << '\n';
      }
    }

    for (auto &lms : LMS) {
      llvm::dbgs() << "checking lms:\n";

      auto res1 = GetIteratorVariance(lms.Target, ItVals, &CurLoop);

      if (lms.Sources.empty()) {
        llvm::dbgs() << "unhandled case because of empty sources\n";
        continue;
      }

      for (auto *src : lms.Sources) {
        auto res2 = GetIteratorVariance(src, ItVals, &CurLoop);

        if (res1 == IteratorVarianceValue::Invariant &&
            res2 == IteratorVarianceValue::Invariant) {
          llvm::dbgs() << "assuming commutativity\n";
          continue;
        }

        if (res1 == IteratorVarianceValue::Variant ||
            res2 == IteratorVarianceValue::Variant) {
          llvm::dbgs() << "check operations to determine commutativity\n";
          continue;
        }

        llvm::dbgs() << "unhandled case\n";
      }
    }

    llvm::dbgs() << "=============\n";
  }
};

} // namespace iteratorrecognition
