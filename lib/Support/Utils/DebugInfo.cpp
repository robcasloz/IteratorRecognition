//
//
//

#include "IteratorRecognition/Support/Utils/DebugInfo.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/IR/DebugInfoMetadata.h"
// using llvm::DISubprogram

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_string_ostream

namespace iteratorrecognition {
namespace dbg {

std::string extract(const llvm::Instruction &I) {
  std::string outs;
  llvm::raw_string_ostream ss(outs);

  ss << I;

  return ss.str();
}

LoopDebugInfoT extract(const llvm::Loop &CurLoop) {
  auto loc = CurLoop.getStartLoc();
  std::string funcName{""}, fileName{""};

  if (auto *sp = llvm::dyn_cast<llvm::DISubprogram>(loc.getScope())) {
    funcName = sp->getName();
    fileName = sp->getFilename();
  }

  return std::make_tuple(loc.getLine(), loc.getCol(), funcName, fileName);
}

} // namespace dbg
} // namespace iteratorrecognition
