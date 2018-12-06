//
//
//

#ifndef ITR_RECOGNIZERPASS_HPP
#define ITR_RECOGNIZERPASS_HPP

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include <memory>
// using std::unique_ptr

namespace llvm {
class Instruction;
class Loop;
class Function;
} // namespace llvm

namespace iteratorrecognition {

class IteratorRecognitionWrapperPass : public llvm::FunctionPass {
  std::unique_ptr<IteratorRecognitionInfo> Info;

public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  IteratorRecognitionWrapperPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &CurFunc) override;

  decltype(auto) getIteratorRecognitionInfo() { return Info.get(); }
  decltype(auto) getIteratorRecognitionInfo() const { return Info.get(); }
};

} // namespace iteratorrecognition

#endif // header
