//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

//#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::Argument
// using llvm::GetElementPtrInst
// using llvm::LoadInst
// using llvm::StoreInst
// using llvm::SelectInst

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <utility>
// using std::move

#define DEBUG_TYPE "itr-iva"

namespace iteratorrecognition {

enum class IteratorVarianceValue { Unknown, Invariant, Variant };

class IteratorVariance {
  IteratorVarianceValue Val;

public:
  explicit IteratorVariance(
      IteratorVarianceValue V = IteratorVarianceValue::Unknown)
      : Val(V) {}

  decltype(auto) get() const { return Val; }

  bool operator==(const IteratorVarianceValue &RhsVal) const {
    return this->Val == RhsVal;
  }
  bool operator==(const IteratorVariance &Rhs) const {
    return this->Val == Rhs.Val;
  }

  bool mergeIn(const IteratorVariance &Other) { return mergeIn(Other.Val); }

  bool mergeIn(const IteratorVarianceValue &OtherVal) {
    auto curVal = Val;

    switch (curVal) {
    case IteratorVarianceValue::Unknown:
      curVal = OtherVal;
      break;
    case IteratorVarianceValue::Invariant:
    case IteratorVarianceValue::Variant:
    default:
      if (OtherVal == IteratorVarianceValue::Variant) {
        curVal = IteratorVarianceValue::Variant;
      }

      break;
    };

    bool hasChanged = (curVal == Val);
    Val = curVal;

    return hasChanged;
  }
};

//

class IteratorVarianceAnalyzer {
  IteratorInfo &Info;
  llvm::DenseMap<llvm::Value *, IteratorVarianceValue> VarianceCache;

public:
  IteratorVarianceAnalyzer() = delete;

  explicit IteratorVarianceAnalyzer(const IteratorInfo &Info)
      : Info(const_cast<IteratorInfo &>(Info)) {}

  void reset() { VarianceCache.clear(); }

  const IteratorInfo &getInfo() const { return Info; }

  IteratorVarianceValue getVariance(const llvm::Value *Query) const {
    llvm::SmallVector<const llvm::Value *, 8> workList{Query};
    llvm::SmallPtrSet<const llvm::Value *, 8> visited;

    IteratorVariance status{};

    while (!workList.empty()) {
      auto *curVal = workList.pop_back_val();

      if (visited.count(curVal)) {
        continue;
      }
      visited.insert(curVal);

      if (status == IteratorVarianceValue::Variant) {
        break;
      }

      if (llvm::isa<llvm::Constant>(curVal) ||
          llvm::isa<llvm::Argument>(curVal)) {
        status.mergeIn(IteratorVarianceValue::Invariant);
        continue;
      }

      auto *curInst = llvm::dyn_cast<llvm::Instruction>(curVal);
      if (!curInst) {
        LLVM_DEBUG(llvm::dbgs() << "Unhandled value: "
                                << strconv::to_string(*curVal) << '\n';);
        status.mergeIn(IteratorVarianceValue::Unknown);
        break;
      }

      if (!Info.getLoop()->contains(curInst)) {
        status.mergeIn(IteratorVarianceValue::Invariant);
        continue;
      }

      if (Info.isIterator(curInst)) {
        status.mergeIn(IteratorVarianceValue::Variant);
        continue;
      }

      if (auto *ii = llvm::dyn_cast<llvm::LoadInst>(curInst)) {
        workList.push_back(ii->getPointerOperand());
      } else if (auto *ii = llvm::dyn_cast<llvm::StoreInst>(curInst)) {
        workList.push_back(ii->getPointerOperand());
      } else if (auto *ii = llvm::dyn_cast<llvm::SelectInst>(curInst)) {
        workList.push_back(ii->getOperand(1));
        workList.push_back(ii->getOperand(2));
      } else if (auto *ii = llvm::dyn_cast<llvm::PHINode>(curInst)) {
        for (auto &op : ii->incoming_values()) {
          workList.push_back(op);
        }
      } else if (auto *ii = llvm::dyn_cast<llvm::BinaryOperator>(curInst)) {
        for (auto &op : ii->operands()) {
          workList.push_back(op.get());
        }
      } else if (auto *ii = llvm::dyn_cast<llvm::CastInst>(curInst)) {
        for (auto &op : ii->operands()) {
          workList.push_back(op.get());
        }
      } else if (auto *ii = llvm::dyn_cast<llvm::GetElementPtrInst>(curInst)) {
        workList.push_back(ii->getPointerOperand());
        for (auto &idx : ii->indices()) {
          workList.push_back(idx.get());
        }
      } else {
        LLVM_DEBUG(llvm::dbgs() << "Unhandled instruction: "
                                << strconv::to_string(*curInst) << '\n';);
      }
    }

    return status.get();
  }

  IteratorVarianceValue getOrInsertVariance(const llvm::Value *Query) {
    auto found = VarianceCache.find(Query);

    if (found == VarianceCache.end()) {
      auto v = getVariance(Query);

      found = VarianceCache
                  .insert(std::make_pair(const_cast<llvm::Value *>(Query), v))
                  .first;
    }

    return (*found).getSecond();
  }
};

} // namespace iteratorrecognition

#undef DEBUG_TYPE
