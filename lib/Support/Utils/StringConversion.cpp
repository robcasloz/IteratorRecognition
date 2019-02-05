//
//
//

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

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

} // namespace strconv
} // namespace iteratorrecognition
