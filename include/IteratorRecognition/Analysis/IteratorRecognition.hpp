//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "Pedigree/Analysis/Graphs/PDGraph.hpp"

#include "IteratorRecognition/Support/Utils/Extras.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/Graphs/PDGCondensationGraph.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo

#include "llvm/IR/ValueHandle.h"
// using llvm::WeakTrackingVH

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

#include "llvm/ADT/Statistic.h"
// using STATISTIC macro

#include "llvm/Support/Casting.h"
// using llvm::cast_or_null

#include <vector>
// using std::vector

#include <algorithm>
// using std::find
// using std::find_if

#include <cassert>
// using assert

namespace iteratorrecognition {

namespace {

using IteratorVectorTy = llvm::SmallVector<llvm::WeakTrackingVH, 8>;

llvm::Instruction *DerefFromVH(IteratorVectorTy::value_type &H) {
  return llvm::cast_or_null<llvm::Instruction>(H);
}

const llvm::Instruction *
ConstDerefFromVH(const IteratorVectorTy::value_type &H) {
  return llvm::cast_or_null<llvm::Instruction>(H);
}

} // namespace

class IteratorInfo {
  llvm::Loop *CurLoop;
  IteratorVectorTy CurInstructions;

public:
  using iterator = llvm::mapped_iterator<typename IteratorVectorTy::iterator,
                                         decltype(&DerefFromVH)>;
  using const_iterator =
      llvm::mapped_iterator<typename IteratorVectorTy::const_iterator,
                            decltype(&ConstDerefFromVH)>;

  using insts_iterator = iterator;
  using const_insts_iterator = const_iterator;

  template <typename IteratorT>
  IteratorInfo(const llvm::Loop *L, IteratorT Begin, IteratorT End)
      : CurLoop(const_cast<llvm::Loop *>(L)), CurInstructions(Begin, End) {}

  explicit IteratorInfo(llvm::Loop *L) : CurLoop(L) {}

  const auto *getLoop() const { return CurLoop; }
  auto getNumInstructions() const { return CurInstructions.size(); }

  decltype(auto) begin() {
    return iterator(CurInstructions.begin(), &DerefFromVH);
  }
  decltype(auto) end() { return iterator(CurInstructions.end(), &DerefFromVH); }

  decltype(auto) begin() const {
    return const_iterator(CurInstructions.begin(), &ConstDerefFromVH);
  }
  decltype(auto) end() const {
    return const_iterator(CurInstructions.end(), &ConstDerefFromVH);
  }

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
    return end() != std::find(begin(), end(), Inst);
  }

  bool isIterator(const llvm::Instruction *Inst) {
    return static_cast<const IteratorInfo *>(this)->isIterator(Inst);
  }
};

//

bool HasPayloadOnlyBlocks(const IteratorInfo &Info, const llvm::Loop &L);

void GetPayloadOnlyBlocks(const IteratorInfo &Info, const llvm::Loop &L,
                          llvm::SmallVectorImpl<llvm::BasicBlock *> &Blocks);

bool HasMixedBlocks(const IteratorInfo &Info, const llvm::Loop &L);

void GetMixedBlocks(const IteratorInfo &Info, const llvm::Loop &L,
                    llvm::SmallVectorImpl<llvm::BasicBlock *> &Mixed);

bool HasPayloadOnlySubloops(const IteratorInfo &Info, const llvm::Loop &L);

void GetPayloadOnlySubloops(const IteratorInfo &Info, const llvm::Loop &L,
                            llvm::SmallVectorImpl<llvm::Loop *> &SubLoops);

//

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

  void MapCondensationToLoops();
  void RecognizeIterator();

public:
  IteratorRecognitionInfo(const llvm::LoopInfo &CurLI, BaseGraphT &CurPDG);

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

  llvm::iterator_range<IteratorInfo::insts_iterator>
  getIteratorsFor(const llvm::Loop *L) const {
    assert(L && "Loop is null!");
    static IteratorVectorTy empty;

    auto found = std::find_if(IteratorsInfo.begin(), IteratorsInfo.end(),
                              [&L](const auto &e) { return e.getLoop() == L; });

    if (found != IteratorsInfo.end()) {
      return const_cast<IteratorInfo &>(*found).insts();
    } else {
      return llvm::make_range(
          IteratorInfo::iterator(empty.begin(), &DerefFromVH),
          IteratorInfo::iterator(empty.end(), &DerefFromVH));
    }
  }

  llvm::iterator_range<IteratorInfo::insts_iterator>
  getIteratorsFor(const llvm::Loop *L) {
    return static_cast<const IteratorRecognitionInfo *>(this)->getIteratorsFor(
        L);
  }

  void print(llvm::raw_ostream &OS) const;

  void dump() const;
};

} // namespace iteratorrecognition

