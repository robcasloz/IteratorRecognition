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
} // namespace llvm

namespace iteratorrecognition {

using LoopDebugInfoT = std::tuple<unsigned, unsigned, std::string, std::string>;

LoopDebugInfoT extractLoopDebugInfo(const llvm::Loop &CurLoop);

} // namespace iteratorrecognition

