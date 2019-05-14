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
// using llvm::AnalysisInfoMixin

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include <memory>
// using std::unique_ptr

#define ITR_RECOGNIZE_PASS_NAME "itr-recognize"

namespace llvm {
class Instruction;
class Loop;
class Function;
} // namespace llvm

namespace iteratorrecognition {

// new passmanager pass
class IteratorRecognitionAnalysis
    : public llvm::AnalysisInfoMixin<IteratorRecognitionAnalysis> {
  friend llvm::AnalysisInfoMixin<IteratorRecognitionAnalysis>;

  static llvm::AnalysisKey Key;

public:
  using Result = IteratorRecognitionInfo;

  IteratorRecognitionAnalysis();
  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
};

// legacy passmanager pass
class IteratorRecognitionWrapperPass : public llvm::FunctionPass {
  std::unique_ptr<IteratorRecognitionInfo> Info;

public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  IteratorRecognitionWrapperPass();

  bool runOnFunction(llvm::Function &CurFunc) override;

  IteratorRecognitionInfo &getIteratorRecognitionInfo() { return *Info; }
  const IteratorRecognitionInfo &getIteratorRecognitionInfo() const {
    return *Info;
  }
};

} // namespace iteratorrecognition

