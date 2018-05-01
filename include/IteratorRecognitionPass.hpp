//
//
//

#ifndef ITR_ITRPASS_HPP
#define ITR_ITRPASS_HPP

#include "llvm/Pass.h"
// using llvm::FunctionPass

namespace llvm {
class Function;
} // namespace llvm

namespace itr {

class IteratorRecognitionPass : public llvm::FunctionPass {
public:
  static char ID;

  IteratorRecognitionPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &CurFunc) override;
};

} // namespace itr

#endif // header
