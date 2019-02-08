//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

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
// using llvm::SmallVectorImpl
// using llvm::SmallVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl
// using llvm::SmallPtrSet

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/Support/JSON.h"
// using json::Value
// using json::Object
// using json::Array

#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/Debug.h"
// using llvm::dbgs

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

// TODO this needs to be moved, too much duplication
decltype(auto) determineHazard(const llvm::Instruction &Src,
                               const llvm::Instruction &Dst) {
  using namespace pedigree;

  BasicDependenceInfo::value_type info{DependenceOrigin::Memory,
                                       DependenceHazard::Unknown};

  if (Src.mayReadFromMemory() && Dst.mayReadFromMemory()) {
    // do not add edge
  } else if (Src.mayReadFromMemory() && Dst.mayWriteToMemory()) {
    info.hazards |= DependenceHazard::Anti;
  } else if (Src.mayWriteToMemory() && Dst.mayReadFromMemory()) {
    info.hazards |= DependenceHazard::Flow;
  } else if (Src.mayWriteToMemory() && Dst.mayWriteToMemory()) {
    info.hazards |= DependenceHazard::Out;
  } else {
    LLVM_DEBUG(llvm::dbgs() << "No appropriate hazard was found!");
  }

  return info;
}

//

class IteratorVarianceAnalyzer {
  IteratorInfo &Info;
  llvm::DenseMap<llvm::Value *, IteratorVarianceValue> VarianceCache;

public:
  IteratorVarianceAnalyzer() = delete;

  IteratorVarianceAnalyzer(const IteratorInfo &Info)
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
        llvm::dbgs() << "Unhandled value: " << strconv::to_string(*curVal)
                     << '\n';
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
        llvm::dbgs() << "Unhandled instruction: "
                     << strconv::to_string(*curInst) << '\n';
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

//

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>>
class IteratorVarianceGraphUpdater {
public:
  template <typename IteratorT>
  IteratorVarianceGraphUpdater(GraphT &G, IteratorT Begin, IteratorT End,
                               IteratorVarianceAnalyzer &IVA,
                               llvm::json::Object *JSONExport = nullptr) {
    llvm::json::Array updates;

    for (auto it = Begin, ei = End; it != ei; ++it) {
      auto &dependence = (*it).first;

      auto res1 = IVA.getOrInsertVariance(dependence.first);
      auto res2 = IVA.getOrInsertVariance(dependence.second);

      auto *srcNode = G.getNode(dependence.first);
      auto *dstNode = G.getNode(dependence.second);

      // unhandled combination
      if (res1 == IteratorVarianceValue::Unknown ||
          res2 == IteratorVarianceValue::Unknown) {
        // TODO maybe be conservative here?

        if (JSONExport) {
          llvm::json::Object updateMapping;
          updateMapping["src"] = strconv::to_string(*dependence.first);
          updateMapping["dst"] = strconv::to_string(*dependence.second);
          updateMapping["remark"] = "unknown relation to iterator";

          updates.push_back(std::move(updateMapping));
        }

        continue;
      }

      if (res1 == IteratorVarianceValue::Variant ||
          res2 == IteratorVarianceValue::Variant) {
        if (srcNode->hasEdgeWith(dstNode)) {
          auto infoOrEmpty = srcNode->getEdgeInfo(dstNode);

          if (infoOrEmpty) {
            auto info = *infoOrEmpty;

            if (info.origins & pedigree::DependenceOrigin::Memory) {
              // FIXME
              // info.origins =
              // static_cast<unsigned>(info.origins) &
              //~static_cast<unsigned>(pedigree::DependenceOrigin::Memory);

              // if (!info.origins) {
              srcNode->removeDependentNode(dstNode);
              //}
            }
          }

          if (JSONExport) {
            llvm::json::Object updateMapping;
            updateMapping["src"] = strconv::to_string(*dependence.first);
            updateMapping["dst"] = strconv::to_string(*dependence.second);
            updateMapping["remark"] = "disconnect because of iterator variance";

            updates.push_back(std::move(updateMapping));
          }
        }

        continue;
      }

      if (res1 == IteratorVarianceValue::Invariant ||
          res2 == IteratorVarianceValue::Invariant) {
        if (!srcNode->hasEdgeWith(dstNode)) {
          auto info = determineHazard(*dependence.first, *dependence.second);

          if (info.hazards) {
            srcNode->addDependentNode(dstNode, info);
          }

          if (JSONExport) {
            llvm::json::Object updateMapping;
            updateMapping["src"] = strconv::to_string(*dependence.first);
            updateMapping["dst"] = strconv::to_string(*dependence.second);
            updateMapping["remark"] = "connect because of iterator invariance";

            updates.push_back(std::move(updateMapping));
          }
        }

        continue;
      }
    }

    if (JSONExport) {
      llvm::json::Object infoMapping;
      infoMapping["loop"] = llvm::toJSON(*IVA.getInfo().getLoop());
      infoMapping["updates"] = std::move(updates);
      (*JSONExport)["loop updates"] = std::move(infoMapping);
    }
  }
};

} // namespace iteratorrecognition
