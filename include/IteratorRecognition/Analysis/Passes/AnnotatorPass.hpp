//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

namespace llvm {
class Function;
} // namespace llvm

namespace iteratorrecognition {

// legacy passmanager pass

class AnnotatorLegacyPass : public llvm::FunctionPass {
public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  AnnotatorLegacyPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &CurFunc) override;
};

} // namespace iteratorrecognition

