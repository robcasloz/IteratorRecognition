//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

namespace llvm {
class Function;
class DominatorTree;
class LoopInfo;
class AAResults;
} // namespace llvm

namespace iteratorrecognition {

class IteratorRecognitionInfo;

class PayloadDependenceGraphLegacyPass : public llvm::FunctionPass {
public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  PayloadDependenceGraphLegacyPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &F) override;

  bool run(llvm::Function &F, llvm::DominatorTree &DT, llvm::LoopInfo &LI,
           llvm::AAResults &AA, IteratorRecognitionInfo &Info);
};

} // namespace iteratorrecognition

