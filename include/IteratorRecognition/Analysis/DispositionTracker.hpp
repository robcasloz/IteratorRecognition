//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/ValueClassification.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::Argument
// using llvm::GetElementPtrInst
// using llvm::LoadInst
// using llvm::StoreInst
// using llvm::SelectInst

#include "llvm/ADT/SetVector.h"
// using llvm::SetVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

namespace iteratorrecognition {

enum class AccessDisposition { Unknown, Invariant, Variant };

//

class DispositionTracker {
  IteratorInfo *Info;

public:
  DispositionTracker() = delete;

  explicit DispositionTracker(const IteratorInfo &Info)
      : Info(&const_cast<IteratorInfo &>(Info)) {}

  const IteratorInfo &getInfo() const { return *Info; }

  AccessDisposition getDisposition(const llvm::Value *Query);
};

} // namespace iteratorrecognition

