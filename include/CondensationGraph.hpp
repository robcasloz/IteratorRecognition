//
//
//

#include "Config.hpp"

#include "Debug.hpp"

#include "Analysis/Graphs/PDGraph.hpp"

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/EquivalenceClasses.h"
// using llvm::EquivalenceClasses

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include <boost/iterator/iterator_facade.hpp>
// using boost::iterator_facade
// using boost::bidirectional_traversal_tag
// using boost::iterator_core_access

#include <vector>
// using std::vector

#include <iterator>
// using std::iterator_traits

#include <type_traits>
// using std::is_trivially_copyable
// using std::is_same

#include <utility>
// using std::forward

#ifndef ITR_CONDENSATIONGRAPH_HPP
#define ITR_CONDENSATIONGRAPH_HPP

namespace itr {

template <typename GraphT>
using CondensationType =
    std::vector<typename llvm::GraphTraits<GraphT>::NodeRef>;

template <typename GraphT>
using CondensationVectorType = std::vector<CondensationType<GraphT>>;

template <typename GraphT>
using ConstCondensationVectorType = std::vector<const CondensationType<GraphT>>;

template <typename NodeRefT, typename NodeT = std::remove_pointer_t<NodeRefT>>
class CondensationGraph {
  static_assert(std::is_trivially_copyable<NodeRefT>::value,
                "NodeRef is not trivially copyable!");

public:
  using NodeType = NodeT;
  using NodeRef = NodeRefT;

private:
  llvm::EquivalenceClasses<NodeRef> Nodes;
  llvm::DenseMap<NodeRef, std::vector<NodeRef>> OutEdges;

  class CondensationGraphIterator
      : public boost::iterator_facade<
            CondensationGraphIterator, const NodeRef,
            typename std::iterator_traits<typename decltype(
                Nodes)::iterator>::iterator_category> {
  public:
    using base_iterator = typename decltype(Nodes)::iterator;

    const CondensationGraph *CurrentCG;
    base_iterator CurrentIt;

    CondensationGraphIterator(const CondensationGraph &CG, bool IsEnd = false)
        : CurrentCG(&CG), CurrentIt(IsEnd ? CG.Nodes.end() : CG.Nodes.begin()) {
    }

  private:
    friend class boost::iterator_core_access;

    void increment() { ++CurrentIt; };
    void decrement() { --CurrentIt; };
    bool equal(const CondensationGraphIterator &Other) const {
      return CurrentCG == Other.CurrentCG && CurrentIt == Other.CurrentIt;
    }

    const NodeRef &dereference() const {
      return *CurrentCG->Nodes.findLeader(CurrentIt);
    }
  };

  template <typename IteratorT>
  void addCondensedNode(IteratorT Begin, IteratorT End) {
    // TODO this might be required to accommodate implicit conversions
    static_assert(
        std::is_same<typename std::iterator_traits<IteratorT>::value_type,
                     NodeRef>::value,
        "Iterator type cannot be dereferenced to the expected value!");

    if (Begin == End)
      return;

    for (auto it = Begin; it != End; ++it)
      Nodes.unionSets(*Begin, *it);
  }

  void connectEdges() {
    llvm::DenseSet<NodeRef> dstLeaders;

    for (auto &srcLeader : nodes()) {
      dstLeaders.clear();

      for (auto &m : scc_members(srcLeader)) {
        for (const auto &n : m->nodes()) {
          auto &dstLeader = *Nodes.findLeader(n);

          if (dstLeader == srcLeader || dstLeaders.count(dstLeader)) {
            continue;
          }

          dstLeaders.insert(dstLeader);
        }

        OutEdges.try_emplace(srcLeader, dstLeaders.begin(), dstLeaders.end());
      }
    }
  }

public:
  explicit CondensationGraph() = default;

  template <typename IteratorT>
  explicit CondensationGraph(IteratorT Begin, IteratorT End) {
    if (Begin == End) {
      return;
    }

    for (auto &scc : llvm::make_range(Begin, End)) {
      addCondensedNode(std::begin(scc), std::end(scc));
    }

    connectEdges();
  }

  CondensationGraph(const CondensationGraph &G) = default;

  decltype(auto) getEntryNode() const {
    return *(Nodes.member_begin(Nodes.begin()));
  }

  decltype(auto) size() const { return Nodes.getNumClasses(); }
  bool empty() const { return Nodes.empty(); }

  using nodes_iterator = CondensationGraphIterator;

  decltype(auto) nodes_begin() const { return nodes_iterator(*this); }
  decltype(auto) nodes_end() const { return nodes_iterator(*this, true); }

  decltype(auto) nodes() const {
    return llvm::make_range(nodes_begin(), nodes_end());
  }

  using scc_members_iterator = typename decltype(Nodes)::member_iterator;

  decltype(auto) scc_members_begin(nodes_iterator It) {
    return Nodes.member_begin(Nodes.findLeader(It));
  }

  decltype(auto) scc_members_begin(NodeRef Elem) {
    return Nodes.findLeader(Elem);
  }

  decltype(auto) scc_members_end() { return Nodes.member_end(); }

  template <typename T> decltype(auto) scc_members(T &&E) {
    return llvm::make_range(scc_members_begin(std::forward<T>(E)),
                            scc_members_end());
  }
};

//

} // namespace itr

namespace itr {

// generic base for easing the task of creating graph traits for graphs

template <typename GraphT> struct LLVMCondensationGraphTraitsHelperBase {
  using NodeType = typename GraphT::NodeType;
  using NodeRef = typename GraphT::NodeRef;

  static NodeRef getEntryNode(GraphT *G) { return G->getEntryNode(); }
  static unsigned size(GraphT *G) { return G->size(); }

  using ChildIteratorType = typename NodeType::nodes_iterator;
  static decltype(auto) child_begin(NodeRef G) { return G->nodes_begin(); }
  static decltype(auto) child_end(NodeRef G) { return G->nodes_end(); }

  static decltype(auto) children(NodeRef G) { return G->nodes(); }

  using nodes_iterator = typename GraphT::nodes_iterator;
  static decltype(auto) nodes_begin(GraphT *G) { return G->nodes_begin(); }
  static decltype(auto) nodes_end(GraphT *G) { return G->nodes_end(); }

  static decltype(auto) nodes(GraphT *G) { return G->nodes(); }
};

} // namespace itr

#endif // header
