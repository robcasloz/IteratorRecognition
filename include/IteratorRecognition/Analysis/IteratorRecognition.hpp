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

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/DenseSet.h"
// using llvm::DenseSet

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "boost/range/adaptors.hpp"
// using boost::adaptors::filtered

#include "boost/range/algorithm.hpp"
// using boost::range::transform

#ifndef ITR_ITERATORRECOGNITION_HPP
#define ITR_ITERATORRECOGNITION_HPP

namespace iteratorrecognition {

// namespace aliases

namespace ba = boost::adaptors;
namespace br = boost::range;

//

template <typename ValueT, typename ValueInfoT>
bool operator==(const llvm::DenseSet<ValueT, ValueInfoT> &LHS,
                const llvm::DenseSet<ValueT, ValueInfoT> &RHS) {
  if (LHS.size() != RHS.size())
    return false;

  for (auto &E : LHS)
    if (!RHS.count(E))
      return false;

  return true;
}

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
  llvm::DenseMap<llvm::Loop *, llvm::SmallVector<llvm::Instruction *, 8>>
      Iterators;

  void MapCondensationToLoops() {
    for (const auto &cn : CGT::nodes(CG)) {
      CondensationToLoopsMapT::mapped_type loops;

      for (const auto &n : *cn | ba::filtered(is_not_null_unit)) {
        loops.insert(LI.getLoopFor(n->unit()->getParent()));
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
        // FIXME the loop set comparison needs to be subset of instead of
        // equality
        if (found != Map.end() && found->second == loops) {
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
          Iterators.insert({loop, instructions});
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
  const auto &getIterators() { return Iterators; }
};

} // namespace iteratorrecognition

#endif // header
