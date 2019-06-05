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

//#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#define DEBUG_TYPE "itr-access-disposition"

namespace iteratorrecognition {

enum class AccessDisposition { Unknown, Invariant, Variant };

//

class DispositionTracker {
  IteratorInfo &Info;
  // llvm::DenseMap<llvm::Value *, AccessDisposition> Cache;

public:
  DispositionTracker() = delete;

  explicit DispositionTracker(const IteratorInfo &Info)
      : Info(const_cast<IteratorInfo &>(Info)) {}

  void reset() {
    // Cache.clear();
  }

  const IteratorInfo &getInfo() const { return Info; }

  // AccessDisposition getDisposition(const llvm::Value *Query) {
  // auto found = Cache.find(Query);

  // if (found == Cache.end()) {
  // auto v = calculateDisposition(Query);

  // found = Cache.insert(std::make_pair(const_cast<llvm::Value *>(Query), v))
  //.first;
  //}

  // return (*found).getSecond();
  //}

  AccessDisposition getDisposition(const llvm::Value *Query) {
    LLVM_DEBUG(llvm::dbgs()
                   << "\nquering : " << strconv::to_string(*Query) << '\n';);

    auto *curInst = llvm::dyn_cast<llvm::Instruction>(Query);
    if (curInst && Info.isIterator(curInst)) {
      return AccessDisposition::Invariant;
    }

    llvm::SmallPtrSet<llvm::Instruction *, 32> payload;
    FindPayloadValues(Info, payload);

    for (auto *curUser : Query->users()) {
      auto *userInst = llvm::dyn_cast<llvm::Instruction>(curUser);
      if (!payload.count(userInst)) {
        continue;
      }

      if (auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(curUser)) {
        for (auto &op : gep->operands()) {
          auto *i = llvm::dyn_cast<llvm::Instruction>(op.get());
          if (i && Info.isIterator(i)) {
            return AccessDisposition::Variant;
          }
        }
      }
    }

    return AccessDisposition::Invariant;
  }
};

} // namespace iteratorrecognition

#undef DEBUG_TYPE

