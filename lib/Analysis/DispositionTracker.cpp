//
//
//

#include "IteratorRecognition/Analysis/DispositionTracker.hpp"

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

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <algorithm>
// using std::any_of

#define DEBUG_TYPE "itr-access-disposition"

namespace iteratorrecognition {

bool DispositionTracker::isIterator(const llvm::Instruction *I,
                                    const llvm::Loop *CurL,
                                    bool ConsiderSubLoopIterators) const {
  if (ConsiderSubLoopIterators) {
    return std::any_of(Infos.begin(), Infos.end(), [I, CurL](const auto &e) {
      auto *l = e.getLoop();
      return (CurL == l || CurL->contains(l)) && e.isIterator(I);
    });
  }

  return std::any_of(Infos.begin(), Infos.end(), [I, CurL](const auto &e) {
    return e.getLoop() == CurL && e.isIterator(I);
  });
}

AccessDisposition
DispositionTracker::getDisposition(const llvm::Value *Query,
                                   const llvm::Loop *L,
                                   bool ConsiderSubLoopIterators) {
  init(const_cast<llvm::Loop *>(L));

  auto *query = const_cast<llvm::Value *>(Query);

  LLVM_DEBUG(llvm::dbgs() << "\nquering : " << strconv::to_string(*query)
                          << '\n';);

  auto *curInst = llvm::dyn_cast<llvm::Instruction>(query);
  if (curInst && isIterator(curInst, L, ConsiderSubLoopIterators)) {
    return AccessDisposition::Invariant;
  }

  auto found = std::find_if(Infos.begin(), Infos.end(),
                            [L](const auto &e) { return e.getLoop() == L; });

  assert(found != Infos.end() && "Missing iterator info!");
  auto &info = *found;

  llvm::SmallPtrSet<llvm::Instruction *, 32> payload;
  FindPayloadValues(info, payload);
  llvm::SetVector<llvm::GetElementPtrInst *> geps;

  for (auto *curUser : query->users()) {
    auto *userInst = llvm::dyn_cast<llvm::Instruction>(curUser);

    if (auto *gep = llvm::dyn_cast_or_null<llvm::GetElementPtrInst>(userInst)) {
      if (payload.count(gep)) {
        geps.insert(gep);
        LLVM_DEBUG(llvm::dbgs() << "inserting gep: " << *gep << "\n";);
      }
    }
  }

  if (geps.empty()) {
    LLVM_DEBUG(llvm::dbgs() << *query
                            << " found to be iterator invariant since there "
                               "are no gep uses\n";);
    return AccessDisposition::Invariant;
  }

  llvm::SmallVector<llvm::Instruction *, 32> workList;
  llvm::SmallPtrSet<llvm::Instruction *, 32> visited;

  for (auto *u : geps) {
    for (auto &op : u->indices()) {
      if (auto *i = llvm::dyn_cast<llvm::Instruction>(op.get())) {
        workList.push_back(i);
      }
    }
  }

  while (!workList.empty()) {
    auto *userInst = workList.pop_back_val();

    if (!visited.insert(userInst).second) {
      continue;
    }

    LLVM_DEBUG(llvm::dbgs() << "checking gep index: " << *userInst << "\n";);

    if (isIterator(userInst, L, ConsiderSubLoopIterators)) {
      LLVM_DEBUG(llvm::dbgs()
                     << *userInst << " found to be iterator variant\n";);
      return AccessDisposition::Variant;
    }

    for (auto &op : userInst->operands()) {
      if (auto *i = llvm::dyn_cast<llvm::Instruction>(op.get())) {
        LLVM_DEBUG(llvm::dbgs()
                       << *i << " operand added for further processing\n";);
        workList.push_back(i);
      }
    }
  }

  LLVM_DEBUG(llvm::dbgs() << *query << " found to be iterator invariant\n";);
  return AccessDisposition::Invariant;
}

} // namespace iteratorrecognition

