//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

#include "llvm/IR/PassManager.h"
// using llvm::FunctionAnalysisManager
// using llvm::PassInfoMixin

namespace llvm {
class Function;
} // namespace llvm

namespace iteratorrecognition {

// new passmanager pass
class AnnotatorPass : public llvm::PassInfoMixin<AnnotatorPass> {
public:
  AnnotatorPass() = default;
  bool run(llvm::Function &F, IteratorRecognitionInfo &Info);

  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &FAM);
};

// legacy passmanager pass
class AnnotatorLegacyPass : public llvm::FunctionPass {
public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  AnnotatorLegacyPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &F) override;
};

} // namespace iteratorrecognition

