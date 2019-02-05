//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include <string>
// using std::string

#include <tuple>
// using std::make_tuple

namespace llvm {
class Loop;
class Instruction;
} // namespace llvm

namespace iteratorrecognition {
namespace dbg {

std::string extract(const llvm::Instruction &I);

using LoopDebugInfoT = std::tuple<unsigned, unsigned, std::string, std::string>;

LoopDebugInfoT extract(const llvm::Loop &CurLoop);

} // namespace dbg
} // namespace iteratorrecognition

