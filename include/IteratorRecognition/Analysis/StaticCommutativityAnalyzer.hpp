//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/IteratorValueTracking.hpp"

#include "IteratorRecognition/Analysis/DetectOperations.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <algorithm>
// using std::all_of

#define DEBUG_TYPE "itr-sca"

namespace iteratorrecognition {

bool isCommutative(const llvm::Instruction &Inst) {
  bool status = false;

  status |= Inst.isBinaryOp() || llvm::isa<llvm::UnaryInstruction>(&Inst);

  LLVM_DEBUG(if (!status) {
    llvm::dbgs() << "instruction: " << strconv::to_string(Inst)
                 << " is not commutative\n";
  });

  return status;
}

class StaticCommutativityAnalyzer {
public:
  StaticCommutativityAnalyzer() = default;

  void analyze(const llvm::SmallVectorImpl<LoadModifyStore> &LMS,
               IteratorVarianceAnalyzer &IVA) {
    LLVM_DEBUG({
      llvm::dbgs() << "=============\n";
      llvm::dbgs() << "loop: " << strconv::to_string(*IVA.getInfo().getLoop())
                   << "\n";

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
    };);

    bool isSet = false;
    bool commutativityCount = false;

    for (auto &lms : LMS) {
      isSet = false;
      commutativityCount = false;

      auto res1 = IVA.getOrInsertVariance(lms.Target);

      for (auto *src : lms.Sources) {
        auto res2 = IVA.getOrInsertVariance(src);

        if (isSet && !commutativityCount) {
          break;
        }

        if (res1 == IteratorVarianceValue::Invariant &&
            res2 == IteratorVarianceValue::Invariant) {
          LLVM_DEBUG(llvm::dbgs() << "assuming commutativity\n";);

          if (!isSet) {
            commutativityCount = true;
            isSet = true;
          }
          continue;
        }

        if (res1 == IteratorVarianceValue::Variant ||
            res2 == IteratorVarianceValue::Variant) {

          if (std::all_of(lms.Operations.begin(), lms.Operations.end(),
                          [](const auto &e) { return isCommutative(*e); })) {
            if (!isSet) {
              commutativityCount = true;
              isSet = true;
            }
          } else {
            isSet = true;
            commutativityCount = false;
          }
          continue;
        }

        isSet = true;
        commutativityCount = false;
        LLVM_DEBUG(llvm::dbgs() << "unhandled case\n";);
      }
    }

    if (isSet && commutativityCount) {
      LLVM_DEBUG(llvm::dbgs() << "+++ commutative\n");
    }
  }
};

} // namespace iteratorrecognition

#undef DEBUG_TYPE
