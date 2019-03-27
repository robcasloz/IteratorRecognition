//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/Extras.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "Pedigree/Analysis/Graphs/PDGraph.hpp"

#include "IteratorRecognition/Analysis/Graphs/PDGCondensationGraph.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo

#include "llvm/ADT/SCCIterator.h"
// using llvm::scc_begin
// using llvm::scc_end

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range
// using llvm::iterator_range

#include "llvm/ADT/Optional.h"
// using llvm::Optional

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include "boost/range/adaptors.hpp"
// using boost::adaptors::filtered

#include "boost/range/algorithm.hpp"
// using boost::range::transform

#include <vector>
// using std::vector

#include <algorithm>
// using std::find
// using std::find_if

#include <cassert>
// using assert

#define DEBUG_TYPE "itr"

namespace iteratorrecognition {

// namespace aliases

namespace ba = boost::adaptors;
namespace br = boost::range;

//

class IteratorInfo {
  llvm::Loop *CurLoop;
  llvm::SmallVector<llvm::Instruction *, 8> CurInstructions;

public:
  using iterator = decltype(CurInstructions)::iterator;
  using const_iterator = decltype(CurInstructions)::const_iterator;

  using insts_iterator = iterator;
  using const_insts_iterator = const_iterator;

  template <typename IteratorT>
  IteratorInfo(const llvm::Loop *L, IteratorT Begin, IteratorT End)
      : CurLoop(const_cast<llvm::Loop *>(L)), CurInstructions(Begin, End) {}

  explicit IteratorInfo(llvm::Loop *L) : CurLoop(L) {}

  const auto *getLoop() const { return CurLoop; }
  const auto &getInstructions() const { return CurInstructions; }
  auto getNumInstructions() const { return CurInstructions.size(); }

  decltype(auto) begin() { return CurInstructions.begin(); }
  decltype(auto) end() { return CurInstructions.end(); }

  decltype(auto) begin() const { return CurInstructions.begin(); }
  decltype(auto) end() const { return CurInstructions.end(); }

  decltype(auto) insts_begin() { return begin(); }
  decltype(auto) insts_end() { return end(); }

  decltype(auto) insts_begin() const { return begin(); }
  decltype(auto) insts_end() const { return end(); }

  decltype(auto) insts() {
    return llvm::make_range(insts_begin(), insts_end());
  }

  decltype(auto) insts() const {
    return llvm::make_range(insts_begin(), insts_end());
  }

  bool isIterator(const llvm::Instruction *Inst) const {
    return CurInstructions.end() !=
           std::find(CurInstructions.begin(), CurInstructions.end(), Inst);
  }

  bool isIterator(const llvm::Instruction *Inst) {
    return static_cast<const IteratorInfo *>(this)->isIterator(Inst);
  }
};

bool HasPayloadOnlyBlocks(const IteratorInfo &Info);

void GetPayloadOnlyBlocks(const IteratorInfo &Info,
                          llvm::SmallVectorImpl<llvm::BasicBlock *> Blocks);

class IteratorRecognitionInfo {
public:
  using BaseGraphT = pedigree::PDGraph;
  using CondensationGraphT = CondensationGraph<BaseGraphT *>;

private:
  BaseGraphT &PDG;
  const llvm::LoopInfo &LI;
  CondensationGraphT CG;

  using CGT = llvm::GraphTraits<CondensationGraphT>;
  using ICGT = llvm::GraphTraits<llvm::Inverse<CondensationGraphT>>;

public:
  using LoopSet = llvm::SmallPtrSet<llvm::Loop *, 4>;
  using CondensationToLoopsMapT =
      llvm::DenseMap<typename CGT::NodeRef, LoopSet>;

private:
  CondensationToLoopsMapT Map;
  std::vector<IteratorInfo> IteratorsInfo;

  void MapCondensationToLoops() {
    for (const auto &cn : CGT::nodes(CG)) {
      CondensationToLoopsMapT::mapped_type loops;

      LLVM_DEBUG(llvm::dbgs() << "condensation: " << cn
                              << " maps to loops with headers: \n";);

      for (const auto &n : *cn | ba::filtered(is_not_null_unit)) {
        auto *loop = LI.getLoopFor(n->unit()->getParent());

        while (loop) {
          loops.insert(loop);
          loop = loop->getParentLoop();
        }
      }

      loops.erase(nullptr);

      LLVM_DEBUG({
        for (const auto *loop : loops) {
          auto *hdr = loop->getHeader();
          llvm::dbgs() << '\t' << hdr << ' ' << hdr->getName() << '\n';
        }
      });

      Map.try_emplace(cn, loops);
    }
  }

  void RecognizeIterator() {
    const auto &loops = const_cast<llvm::LoopInfo &>(LI).getLoopsInPreorder();

    for (const auto *loop : loops) {
      llvm::SmallPtrSet<CGT::NodeRef, 8> loopCondensations;

      for (auto &e : Map) {
        if (e.second.count(loop)) {
          loopCondensations.insert(e.first);
        }
      }

      llvm::SmallVector<llvm::Instruction *, 8> inst;

      for (auto *cn : loopCondensations) {
        bool workFound = false;

        for (auto &ccn : ICGT::children(cn)) {
          if (loopCondensations.count(ccn)) {
            workFound = true;
            break;
          }
        }

        if (!workFound) {
          br::transform(*cn | ba::filtered(is_not_null_unit),
                        std::back_inserter(inst),
                        [&](const auto &e) { return e->unit(); });
        }
      }

      unique_inplace(inst);
      IteratorsInfo.emplace_back(loop, inst.begin(), inst.end());
    }

    /*LoopSet visited;
    for (const auto &e : Map) {
      const auto &loops = e.second;

      if (loops.empty()) {
        continue;
      }

      const auto &key = e.first;
      bool workFound = false;

      for (auto &cn : ICGT::children(key)) {
        auto found = Map.find(cn);
        if (found != Map.end() && is_subset_of(loops, found->second)) {
          workFound = true;
          break;
        }
      }

      if (!workFound) {
        for (auto *loop : loops) {
          if (visited.count(loop)) {
            continue;
          }

          llvm::SmallVector<llvm::Instruction *, 8> instructions;
          br::transform(*key | ba::filtered(is_not_null_unit),
                        std::back_inserter(instructions),
                        [&](const auto &e) { return e->unit(); });
          IteratorsInfo.emplace_back(loop, instructions.begin(),
                                     instructions.end());
          visited.insert(loop);
        }
      }
    }*/
  }

public:
  IteratorRecognitionInfo(const llvm::LoopInfo &CurLI, BaseGraphT &CurPDG)
      : LI(CurLI), PDG(CurPDG), CG{llvm::scc_begin(&PDG), llvm::scc_end(&PDG)} {
    MapCondensationToLoops();
    RecognizeIterator();
  }

  const auto &getLoopInfo() { return LI; }
  auto &getGraph() { return PDG; }
  const auto &getCondensationGraph() { return CG; }
  const auto &getCondensationGraph() const { return CG; }
  const auto &getCondensationToLoopsMap() { return Map; }
  const auto &getCondensationToLoopsMap() const { return Map; }
  const auto &getIteratorsInfo() { return IteratorsInfo; }
  const auto &getIteratorsInfo() const { return IteratorsInfo; }

  llvm::Optional<IteratorInfo> getIteratorInfoFor(const llvm::Loop *L) const {
    assert(L && "Loop is null!");

    auto found = std::find_if(IteratorsInfo.begin(), IteratorsInfo.end(),
                              [&L](const auto &e) { return e.getLoop() == L; });

    if (found != IteratorsInfo.end()) {
      return *found;
    } else {
      return {};
    }
  }

  llvm::Optional<llvm::iterator_range<IteratorInfo::insts_iterator>>
  getIteratorsFor(const llvm::Loop *L) const {
    assert(L && "Loop is null!");

    auto found = std::find_if(IteratorsInfo.begin(), IteratorsInfo.end(),
                              [&L](const auto &e) { return e.getLoop() == L; });

    if (found != IteratorsInfo.end()) {
      return const_cast<IteratorInfo &>(*found).insts();
    } else {
      return {};
    }
  }

  llvm::Optional<llvm::iterator_range<IteratorInfo::insts_iterator>>
  getIteratorsFor(const llvm::Loop *L) {
    return static_cast<const IteratorRecognitionInfo *>(this)->getIteratorsFor(
        L);
  }
};

} // namespace iteratorrecognition

