//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/ShadowDependenceGraph.hpp"

#include "IteratorRecognition/Analysis/DependenceCache.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

#include "llvm/ADT/SetVector.h"
// using llvm::SetVector

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_string_ostream

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <string>
// using std::string

#include <utility>
// using std::pair
// using std::make_pair

#define DEBUG_TYPE "itr-detectops"

namespace iteratorrecognition {

struct LoadModifyStore {
  llvm::Instruction *Target;
  llvm::SmallPtrSet<llvm::Instruction *, 8> Sources;
  llvm::SmallPtrSet<llvm::Instruction *, 16> Operations;
};

template <typename GraphT> class MutationDetector {
  DependenceCache DC;
  const SDependenceGraph<GraphT> *SDG = nullptr;
  const SDependenceGraphNode<GraphT> *curTargetNode = nullptr;
  llvm::SetVector<llvm::Value *> workList;

  bool processCall(llvm::CallInst &CI) {
    auto found = DC.contains(CI);

    // no side-effects call
    if (!found) {
      for (auto &e : CI.arg_operands()) {
        LLVM_DEBUG(llvm::dbgs() << *e << '\n';);
        workList.insert(e);
      }
      return true;
    }

    return false;
  }

public:
  explicit MutationDetector(const DependenceCache &DC) : DC(DC) {}

  std::pair<bool, std::string> process(const SDependenceGraph<GraphT> &SDG,
                                       const llvm::Loop &CurLoop,
                                       LoadModifyStore &LMS) {
    bool wasProcessed = true;
    std::string message;

    workList.clear();
    curTargetNode = nullptr;
    this->SDG = &SDG;

    llvm::SmallPtrSet<llvm::Value *, 32> visited;
    workList.insert(LMS.Target);

    while (!workList.empty()) {
      auto *curTarget =
          llvm::dyn_cast<llvm::Instruction>(workList.pop_back_val());

      if (!curTarget || visited.count(curTarget)) {
        continue;
      }
      visited.insert(curTarget);

      curTargetNode = SDG.findNodeFor(curTarget);
      if (!curTargetNode) {
        continue;
      }

      llvm::SmallPtrSet<llvm::Value *, 16> inverse_edge_units;
      for (auto *ie : curTargetNode->inverse_edges()) {
        for (auto *iu : ie->units()) {
          inverse_edge_units.insert(iu);
        }
      }

      if (auto *ii = llvm::dyn_cast<llvm::StoreInst>(curTarget)) {
        // TODO maybe this should be the next target
        if (inverse_edge_units.count(ii->getValueOperand())) {
          workList.insert(ii->getValueOperand());
        }
      } else if (auto *ii = llvm::dyn_cast<llvm::LoadInst>(curTarget)) {
        // TODO maybe this should be the next target
        LMS.Sources.insert(ii);
      } else if (auto *ii = llvm::dyn_cast<llvm::BinaryOperator>(curTarget)) {
        for (auto &op : ii->operands()) {
          if (inverse_edge_units.count(op.get())) {
            workList.insert(op);
          }
        }
        LMS.Operations.insert(ii);
      } else if (auto *ii = llvm::dyn_cast<llvm::CmpInst>(curTarget)) {
        for (auto &op : ii->operands()) {
          if (inverse_edge_units.count(op.get())) {
            workList.insert(op);
          }
        }
        LMS.Operations.insert(ii);
      } else if (auto *ii = llvm::dyn_cast<llvm::SelectInst>(curTarget)) {
        if (inverse_edge_units.count(ii->getTrueValue())) {
          workList.insert(ii->getTrueValue());
        }
        if (inverse_edge_units.count(ii->getFalseValue())) {
          workList.insert(ii->getFalseValue());
        }
        // do not insert as operation
        // skip condition
        // just aggregate the operands to unify the cfg
      } else if (auto *ii =
                     llvm::dyn_cast<llvm::GetElementPtrInst>(curTarget)) {
        for (auto &op : ii->operands()) {
          if (inverse_edge_units.count(op.get())) {
            workList.insert(op);
          }
        }
        LMS.Operations.insert(ii);
      } else if (auto *ii = llvm::dyn_cast<llvm::CastInst>(curTarget)) {
        for (auto &op : ii->operands()) {
          if (inverse_edge_units.count(op.get())) {
            workList.insert(op.get());
          }
        }
        LMS.Operations.insert(ii);
      } else if (auto *ii = llvm::dyn_cast<llvm::PHINode>(curTarget)) {
        if (ii->getBasicBlockIndex(CurLoop.getHeader()) > -1) {
          LMS.Sources.insert(ii);
        } else {
          for (auto &op : ii->incoming_values()) {
            if (inverse_edge_units.count(op)) {
              workList.insert(op);
            }
          }
          LMS.Operations.insert(ii);
        }
      } else if (auto *ii = llvm::dyn_cast<llvm::CallInst>(curTarget)) {
        wasProcessed = processCall(*ii);

        if (!wasProcessed) {
          llvm::raw_string_ostream os(message);
          os << "Unhandled call: " << *curTarget;

          LLVM_DEBUG(llvm::dbgs() << message << '\n';);
          break;
        }
      } else {
        wasProcessed = false;
        llvm::raw_string_ostream os(message);
        os << "Unhandled instruction: " << *curTarget;

        LLVM_DEBUG(llvm::dbgs() << message << '\n';);
        break;
      }
    } // work list iteration end

    LMS.Operations.erase(LMS.Target);

    return std::make_pair(wasProcessed, message);
  }
};

} // namespace iteratorrecognition

#undef DEBUG_TYPE
