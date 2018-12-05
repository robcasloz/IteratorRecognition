//
//
//

#ifndef ITR_JSONEXPORTERPASS_HPP
#define ITR_JSONEXPORTERPASS_HPP

#include "IteratorRecognition/Config.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

namespace llvm {
class Function;
} // namespace llvm

namespace iteratorrecognition {

class JSONExporterPass : public llvm::FunctionPass {
public:
  static char ID;

  JSONExporterPass();
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  bool runOnFunction(llvm::Function &CurFunc) override;
};

} // namespace iteratorrecognition

#endif // header
