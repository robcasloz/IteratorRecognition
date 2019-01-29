//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl

namespace llvm {
class Instruction;
class DominatorTree;
} // namespace llvm

namespace iteratorrecognition {

class IteratorInfo;

void FindIteratorValues(const IteratorInfo &Info,
                        llvm::SmallPtrSetImpl<llvm::Instruction *> &Values);

void FindPayloadValues(const IteratorInfo &Info,
                       llvm::SmallPtrSetImpl<llvm::Instruction *> &Values);

void FindMemPayloadLiveValues(
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &Payload,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &MemLiveInThru,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &MemLiveOut);

void FindVirtRegPayloadLiveValues(
    const IteratorInfo &Info,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &Payload,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLive);

void SplitVirtRegPayloadLiveValues(
    const IteratorInfo &Info,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &Payload,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLive,
    const llvm::DominatorTree &DT,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLiveIn,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLiveThru,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLiveOut);

void FindDirectUsesOfIn(
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &Values,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &OtherValues,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &DirectUserValues);

} // namespace iteratorrecognition
