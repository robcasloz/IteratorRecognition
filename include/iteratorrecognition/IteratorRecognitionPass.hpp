//
//
//

#ifndef ITR_ITERATORRECOGNITIONPASS_HPP
#define ITR_ITERATORRECOGNITIONPASS_HPP

#include "iteratorrecognition/Config.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

namespace llvm {
class Function;
} // namespace llvm

namespace iteratorrecognition {

class IteratorRecognitionPass : public llvm::FunctionPass {
public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  IteratorRecognitionPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &CurFunc) override;
};

} // namespace iteratorrecognition

#endif // header
