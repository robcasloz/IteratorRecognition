//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

#include "llvm/IR/PassManager.h"
// using llvm::FunctionAnalysisManager
// using llvm::AnalysisInfoMixin

namespace llvm {
class Function;
class DominatorTree;
class LoopInfo;
class AAResults;
} // namespace llvm

namespace iteratorrecognition {

class IteratorRecognitionInfo;

// new passmanager pass
class PayloadDependenceGraphAnalysis
    : public llvm::AnalysisInfoMixin<PayloadDependenceGraphAnalysis> {
  friend AnalysisInfoMixin<PayloadDependenceGraphAnalysis>;

  static llvm::AnalysisKey Key;

public:
  // TODO change this to something meaningful
  using Result = void;

  PayloadDependenceGraphAnalysis() = default;

  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);

  bool run(llvm::Function &F, llvm::DominatorTree &DT, llvm::LoopInfo &LI,
           llvm::AAResults &AA, IteratorRecognitionInfo &Info);
};

// legacy passmanager pass
class PayloadDependenceGraphLegacyPass : public llvm::FunctionPass {
public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  PayloadDependenceGraphLegacyPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &F) override;
};

} // namespace iteratorrecognition

