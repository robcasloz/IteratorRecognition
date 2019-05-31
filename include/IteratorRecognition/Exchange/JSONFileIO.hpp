//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/Support/JSON.h"
// using json::Value
// using json::Object
// using json::Array

//

namespace iteratorrecognition {

void WriteJSONToFile(const llvm::json::Value &V,
                     const llvm::Twine &FilenamePrefix, const llvm::Twine &Dir);

} // namespace iteratorrecognition

