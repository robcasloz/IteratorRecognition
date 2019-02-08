//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include <string>
// using std::string

namespace llvm {
class Value;
class Instruction;
class Loop;
} // namespace llvm

namespace iteratorrecognition {
namespace strconv {

std::string to_string(const llvm::Value &);
std::string to_string(const llvm::Instruction &);
std::string to_string(const llvm::Loop &);

} // namespace strconv
} // namespace iteratorrecognition
