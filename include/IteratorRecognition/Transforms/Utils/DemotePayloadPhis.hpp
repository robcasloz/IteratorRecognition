//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVectorImpl

namespace llvm {
class Instruction;
class PHINode;
} // namespace llvm

namespace iteratorrecognition {

class IteratorInfo;

bool DemotePhiPayloadValues(
    const IteratorInfo &Info, llvm::SmallPtrSetImpl<llvm::Instruction *> &Phis,
    llvm::SmallVectorImpl<llvm::Instruction *> *Allocas = nullptr);

} // namespace iteratorrecognition

