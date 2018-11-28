//
//
//

#ifndef ITR_ITERATORRECOGNITIONMETADATAANNOTATORPASS_HPP
#define ITR_ITERATORRECOGNITIONMETADATAANNOTATORPASS_HPP

#include "IteratorRecognition/Config.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

namespace llvm {
class Function;
} // namespace llvm

namespace iteratorrecognition {

class IteratorRecognitionMetadataAnnotatorPass : public llvm::FunctionPass {
public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  IteratorRecognitionMetadataAnnotatorPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &CurFunc) override;
};

} // namespace iteratorrecognition

#endif // header
