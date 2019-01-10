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

class PayloadDependenceGraphPass : public llvm::FunctionPass {
public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  PayloadDependenceGraphPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &CurFunc) override;
};

} // namespace iteratorrecognition

