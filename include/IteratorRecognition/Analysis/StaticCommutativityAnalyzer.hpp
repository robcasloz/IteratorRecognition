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

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include "llvm/ADT/DenseSet.h"
// using llvm::SmallDenseSet

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <algorithm>
// using std::all_of

#include <string>
// using std::string

#include <utility>
// using std::pair
// using std::make_pair

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

  std::pair<bool, std::string>
  analyze(const llvm::SmallVectorImpl<LoadModifyStore> &LMS,
          IteratorVarianceAnalyzer &IVA) {
    LLVM_DEBUG({
      llvm::dbgs() << "SCA processing: \n";
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
      llvm::dbgs() << "---\n";
    };);

    bool hasCommutativeOps = false;
    bool stopProcessing = false;
    llvm::SmallDenseSet<llvm::StringRef> usedMessages;

    for (auto &lms : LMS) {
      if (stopProcessing || lms.Sources.empty()) {
        hasCommutativeOps = false;
        llvm::StringRef msg = "non-commutativity due to empty sources";
        usedMessages.insert(msg);

        LLVM_DEBUG(llvm::dbgs() << msg << '\n';);
        break;
      }

      auto res1 = IVA.getOrInsertVariance(lms.Target);

      for (auto *src : lms.Sources) {
        auto res2 = IVA.getOrInsertVariance(src);

        if (res1 == IteratorVarianceValue::Invariant &&
            res2 == IteratorVarianceValue::Invariant) {
          hasCommutativeOps = true;
          llvm::StringRef msg =
              "assuming commutativity due to iterator variance";
          usedMessages.insert(msg);

          LLVM_DEBUG(llvm::dbgs() << msg << '\n';);
          continue;
        }

        if (res1 == IteratorVarianceValue::Variant ||
            res2 == IteratorVarianceValue::Variant) {
          if (std::all_of(lms.Operations.begin(), lms.Operations.end(),
                          [](const auto &e) { return isCommutative(*e); })) {
            hasCommutativeOps = true;
            llvm::StringRef msg = "commutativity due to operations";
            usedMessages.insert(msg);

            LLVM_DEBUG(llvm::dbgs() << msg << '\n';);
          } else {
            hasCommutativeOps = false;
            llvm::StringRef msg = "non-commutativity due to operations";
            usedMessages.insert(msg);
            stopProcessing = true;

            LLVM_DEBUG(llvm::dbgs() << msg << '\n';);
            break;
          }

          continue;
        }

        hasCommutativeOps = false;
        llvm::StringRef msg = "non-commutativity due to unhandled case";
        usedMessages.insert(msg);
        stopProcessing = true;

        LLVM_DEBUG(llvm::dbgs() << msg << '\n';);
        break;
      }
    }

    std::string message;
    for (auto &msg : usedMessages) {
      message += " " + msg.str();
    }

    return std::make_pair(hasCommutativeOps, message);
  }
};

} // namespace iteratorrecognition

#undef DEBUG_TYPE
