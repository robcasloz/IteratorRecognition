//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl

namespace llvm {
class Instruction;
} // namespace llvm

namespace iteratorrecognition {

class IteratorInfo;

void FindIteratorVars(const IteratorInfo &Info,
                      llvm::SmallPtrSetImpl<llvm::Instruction *> &Values);

void FindPayloadVars(const IteratorInfo &Info,
                     llvm::SmallPtrSetImpl<llvm::Instruction *> &Values);

void FindMemPayloadVars(
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &PayloadValues,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &MemPayloadValues);

void FindPayloadTempAndLiveVars(
    const IteratorInfo &Info,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &PayloadValues,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &TempValues,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &LiveValues);

void FindDirectUsesOfIn(
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &Values,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &OtherValues,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &DirectUserValues);

} // namespace iteratorrecognition
