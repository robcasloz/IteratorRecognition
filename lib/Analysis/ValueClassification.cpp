//
//
//

#include "IteratorRecognition/Analysis/ValueClassification.hpp"

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Support/Utils/InstTraversal.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::PHINode

#include "llvm/IR/Dominators.h"
// using llvm::DominatorTree

namespace iteratorrecognition {

void FindIteratorVars(const IteratorInfo &Info,
                      llvm::SmallPtrSetImpl<llvm::Instruction *> &Values) {
  auto &loopBlocks = Info.getLoop()->getBlocksSet();

  for (auto *e : Info) {
    for (auto &u : e->uses()) {
      auto *user = llvm::dyn_cast<llvm::Instruction>(u.getUser());
      if (user && !Info.isIterator(user) &&
          loopBlocks.count(user->getParent())) {
        Values.insert(e);
      }
    }
  }
}

void FindPayloadVars(const IteratorInfo &Info,
                     llvm::SmallPtrSetImpl<llvm::Instruction *> &Values) {
  auto loopInsts = make_loop_inst_range(Info.getLoop());

  for (auto &e : loopInsts) {
    if (!Info.isIterator(&e)) {
      Values.insert(&e);
    }
  }
}

void FindMemPayloadLiveVars(
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &PayloadValues,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &MemLiveInThru,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &MemLiveOut) {
  for (const auto &e : PayloadValues) {
    if (e->mayReadFromMemory()) {
      if (e->mayWriteToMemory()) {
        llvm::report_fatal_error("Instruction cannot also write to memory!");
      }
      MemLiveInThru.insert(e);
    }

    if (e->mayWriteToMemory()) {
      if (e->mayReadFromMemory()) {
        llvm::report_fatal_error("Instruction cannot also read from memory!");
      }
      MemLiveOut.insert(e);
    }
  }
}

void FindVirtRegPayloadLiveVars(
    const IteratorInfo &Info,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &PayloadValues,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLive) {
  // auto &loopBlocks = Info.getLoop()->getBlocksSet();
  llvm::SmallPtrSet<llvm::Instruction *, 32> visited;

  for (const auto &e : PayloadValues) {
    visited.insert(e);
    bool hasAllUsesIn = true;

    for (auto &u : e->uses()) {
      auto *user = llvm::dyn_cast<llvm::Instruction>(u.getUser());
      if (user && !PayloadValues.count(user)) {
        hasAllUsesIn = false;
        break;
      }
    }

    if (!hasAllUsesIn) {
      VirtRegLive.insert(e);
    }

    for (auto &u : e->operands()) {
      auto *opi = llvm::dyn_cast<llvm::Instruction>(u);
      if (opi && !visited.count(opi) && !PayloadValues.count(opi)) {
        visited.insert(opi);
        VirtRegLive.insert(opi);
      }
    }
  }
}

void SplitVirtRegPayloadLiveVars(
    const IteratorInfo &Info,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &PayloadValues,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLive,
    const llvm::DominatorTree &DT,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLiveIn,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLiveThru,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &VirtRegLiveOut) {
  // auto &loopBlocks = Info.getLoop()->getBlocksSet();

  for (auto *e : VirtRegLive) {
    if (PayloadValues.count(e)) {
      VirtRegLiveOut.insert(e);
      continue;
    }

    bool hasUsesAfter = false;
    for (auto &u : e->uses()) {
      auto *user = llvm::dyn_cast<llvm::Instruction>(u.getUser());
      if (user && !PayloadValues.count(user)) {
        if (DT.dominates(e, user) || llvm::isa<llvm::PHINode>(user)) {
          hasUsesAfter = true;
          VirtRegLiveThru.insert(e);
          break;
        }
      }
    }

    if (!hasUsesAfter) {
      VirtRegLiveIn.insert(e);
    }
  }
}

void FindDirectUsesOfIn(
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &Values,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &OtherValues,
    llvm::SmallPtrSetImpl<llvm::Instruction *> &DirectUserValues) {
  for (const auto &e : Values) {
    for (auto &u : e->uses()) {
      auto *user = llvm::dyn_cast<llvm::Instruction>(u.getUser());
      if (user && OtherValues.count(user)) {
        DirectUserValues.insert(user);
      }
    }
  }
}

} // namespace iteratorrecognition
