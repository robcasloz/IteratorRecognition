//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/IteratorValueTracking.hpp"

#include "IteratorRecognition/Analysis/DetectOperations.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

namespace iteratorrecognition {

class StaticCommutativityAnalyzer {
public:
  StaticCommutativityAnalyzer() = default;

  void analyze(const llvm::SmallVectorImpl<LoadModifyStore> &LMS,
               IteratorVarianceAnalyzer &IVA) {
    llvm::dbgs() << "=============\n";
    llvm::dbgs() << "loop: " << strconv::to_string(*IVA.getInfo().getLoop()) << "\n";

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

      auto res1 = IVA.getOrInsertVariance(lms.Target);

      if (lms.Sources.empty()) {
        llvm::dbgs() << "unhandled case because of empty sources\n";
        continue;
      }

      for (auto *src : lms.Sources) {
        auto res2 = IVA.getOrInsertVariance(src);

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
