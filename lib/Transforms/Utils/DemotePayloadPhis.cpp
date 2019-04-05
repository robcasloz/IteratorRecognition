//
//
//

#include "IteratorRecognition/Transforms/Utils/DemotePayloadPhis.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::PHINode

#include "llvm/Transforms/Utils/Local.h"
// using llvm::DemotePHIToStack

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include <cassert>
// using assert

namespace iteratorrecognition {

bool DemotePhiPayloadValues(
    const IteratorInfo &Info, llvm::SmallPtrSetImpl<llvm::Instruction *> &Phis,
    llvm::SmallVectorImpl<llvm::Instruction *> *Allocas) {
  bool hasChanged = false;
  auto *entryPoint =
      Info.getLoop()->getHeader()->getParent()->getEntryBlock().getTerminator();

  for (auto *e : Phis) {
    auto *phi = llvm::dyn_cast<llvm::PHINode>(e);
    if (!phi) {
      continue;
    }

    assert(!Info.isIterator(phi) && "PHI value is not payload!");

    auto *ai = llvm::DemotePHIToStack(phi, entryPoint);
    hasChanged |= ai ? true : false;

    if (Allocas && ai) {
      Allocas->push_back(ai);
    }
  }

  return hasChanged;
}

} // namespace iteratorrecognition

