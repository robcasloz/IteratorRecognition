//
//
//

#ifndef ITR_RECOGNIZERPASS_HPP
#define ITR_RECOGNIZERPASS_HPP

#include "IteratorRecognition/Config.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

namespace llvm {
class Instruction;
class Loop;
class Function;
} // namespace llvm

namespace iteratorrecognition {

class RecognizerPass : public llvm::FunctionPass {
  llvm::DenseMap<llvm::Loop *, llvm::SmallVector<llvm::Instruction *, 8>>
      Iterators;

public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  RecognizerPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &CurFunc) override;

  decltype(Iterators) getIterators();
};

} // namespace iteratorrecognition

#endif // header
