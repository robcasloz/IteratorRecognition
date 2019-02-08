//
//
//

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_string_ostream

namespace iteratorrecognition {
namespace strconv {

std::string to_string(const llvm::Instruction &I) {
  std::string outs;
  llvm::raw_string_ostream ss(outs);

  ss << I;

  return ss.str();
}

std::string to_string(const llvm::Loop &L) { return L.getHeader()->getName(); }

} // namespace strconv
} // namespace iteratorrecognition
