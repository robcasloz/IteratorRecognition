//
//
//

#include "IteratorRecognition/Support/Utils/DebugInfo.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/IR/DebugInfoMetadata.h"
// using llvm::DISubprogram

namespace iteratorrecognition {

LoopDebugInfoT extractLoopDebugInfo(const llvm::Loop &CurLoop) {
  auto loc = CurLoop.getStartLoc();
  std::string funcName{""}, fileName{""};

  if (auto *sp = llvm::dyn_cast<llvm::DISubprogram>(loc.getScope())) {
    funcName = sp->getName();
    fileName = sp->getFilename();
  }

  return std::make_tuple(loc.getLine(), loc.getCol(), funcName, fileName);
}

} // namespace iteratorrecognition
