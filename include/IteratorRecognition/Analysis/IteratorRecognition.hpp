//
//
//

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

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/DenseSet.h"
// using llvm::DenseSet

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range
// using llvm::iterator_range

#include "llvm/ADT/Optional.h"
// using llvm::Optional

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

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

#ifndef ITR_ITERATORRECOGNITION_HPP
#define ITR_ITERATORRECOGNITION_HPP

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

  decltype(auto) begin() const { return CurInstructions.begin(); }
  decltype(auto) end() const { return CurInstructions.end(); }

  decltype(auto) insts_begin() const { return begin(); }
  decltype(auto) insts_end() const { return end(); }

  decltype(auto) insts() const {
    return llvm::make_range(insts_begin(), insts_end());
  }

  bool isIterator(const llvm::Instruction *Inst) const {
    return CurInstructions.end() !=
           std::find(CurInstructions.begin(), CurInstructions.end(), Inst);
  }
};

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
  using CondensationToLoopsMapT =
      llvm::DenseMap<typename CGT::NodeRef, llvm::DenseSet<llvm::Loop *>>;

private:
  CondensationToLoopsMapT Map;
  std::vector<IteratorInfo> IteratorsInfo;

  void MapCondensationToLoops() {
    for (const auto &cn : CGT::nodes(CG)) {
      CondensationToLoopsMapT::mapped_type loops;

      for (const auto &n : *cn | ba::filtered(is_not_null_unit)) {
        auto *loop = LI.getLoopFor(n->unit()->getParent());

        while (loop) {
          loops.insert(loop);
          loop = loop->getParentLoop();
        }
      }

      loops.erase(nullptr);
      Map.try_emplace(cn, loops);
    }
  }

  void RecognizeIterator() {
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
        for (auto &loop : loops) {
          llvm::SmallVector<llvm::Instruction *, 8> instructions;
          br::transform(*key | ba::filtered(is_not_null_unit),
                        std::back_inserter(instructions),
                        [&](const auto &e) { return e->unit(); });
          IteratorsInfo.emplace_back(loop, instructions.begin(),
                                     instructions.end());
        }
      }
    }
  }

public:
  IteratorRecognitionInfo() = delete;
  IteratorRecognitionInfo(const IteratorRecognitionInfo &) = delete;

  IteratorRecognitionInfo(const llvm::LoopInfo &CurLI, BaseGraphT &CurPDG)
      : LI(CurLI), PDG(CurPDG), CG{llvm::scc_begin(&PDG), llvm::scc_end(&PDG)} {
    MapCondensationToLoops();
    RecognizeIterator();
  }

  const auto &getCondensationGraph() { return CG; }
  const auto &getCondensationToLoopsMap() { return Map; }
  const auto &getIteratorsInfo() { return IteratorsInfo; }

  llvm::Optional<llvm::iterator_range<IteratorInfo::const_insts_iterator>>
  getIteratorsFor(const llvm::Loop *L) {
    assert(L && "Loop is null!");

    auto found = std::find_if(IteratorsInfo.begin(), IteratorsInfo.end(),
                              [&L](const auto &e) { return e.getLoop() == L; });

    if (found != IteratorsInfo.end()) {
      return found->insts();
    } else {
      return {};
    }
  }
};

} // namespace iteratorrecognition

#endif // header
