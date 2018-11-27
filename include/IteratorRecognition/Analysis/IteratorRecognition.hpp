//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/Extras.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/DenseSet.h"
// using llvm::DenseSet

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "boost/range/adaptors.hpp"
// using boost::adaptors::filtered

#ifndef ITR_ITERATORRECOGNITION_HPP
#define ITR_ITERATORRECOGNITION_HPP

namespace iteratorrecognition {

// namespace aliases

namespace ba = boost::adaptors;

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>>
void MapCondensationToLoop(
    GraphT &G, const llvm::LoopInfo &LI,
    llvm::DenseMap<typename GT::NodeRef, llvm::DenseSet<llvm::Loop *>> &Map) {
  for (const auto &cn : GT::nodes(G)) {
    typename std::remove_reference_t<decltype(Map)>::mapped_type loops;

    for (const auto &n : *cn | ba::filtered(is_not_null_unit)) {
      loops.insert(LI.getLoopFor(n->unit()->getParent()));
    }

    loops.erase(nullptr);
    Map.try_emplace(cn, loops);
  }
}

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

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>,
          typename IGT = llvm::GraphTraits<llvm::Inverse<GraphT>>>
void RecognizeIterator(
    const GraphT &G,
    const llvm::DenseMap<typename GT::NodeRef, llvm::DenseSet<llvm::Loop *>>
        &Map,
    llvm::DenseSet<typename GraphT::MemberNodeRef> &Iterator) {
  for (const auto &e : Map) {
    const auto &key = e.first;
    const auto &loops = e.second;
    bool workFound = false;

    if (!loops.size()) {
      continue;
    }

    for (auto &cn : IGT::children(key)) {
      auto found = Map.find(cn);

      if (found == Map.end()) {
        continue;
      }

      const auto &cnLoops = found->getSecond();

      if (cnLoops.empty()) {
        continue;
      }

      if (cnLoops == loops) {
        workFound = true;
        break;
      }
    }

    if (!workFound) {
      for (const auto &e : *key | ba::filtered(is_not_null_unit)) {
        Iterator.insert(e);
      }
    }
  }
}

} // namespace iteratorrecognition

#endif // header
