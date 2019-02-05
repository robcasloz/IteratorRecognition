//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include <string>
// using std::string

namespace llvm {
class Instruction;
} // namespace llvm

namespace iteratorrecognition {
namespace strconv {

std::string to_string(const llvm::Instruction &I);

} // namespace strconv
} // namespace iteratorrecognition
