//
//
//

#include "iteratorrecognition/Config.hpp"

#include "iteratorrecognition/Debug.hpp"

#include "pedigree/Analysis/Graphs/PDGraph.hpp"

#include "llvm/ADT/DenseSet.h"
// using llvm::DenseSet

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include <vector>
// using std::vector

#include <map>
// using std::map

#include <iterator>
// using std::iterator_traits
// using std::back_inserter

#include <algorithm>
// using std::copy
// using std::find
// using std::sort
// using std::unique
// using std::for_each
// using std::set_difference

#include <memory>
// using std::addressof

#include <type_traits>
// using std::is_same
// using std::is_const
// using std::conditional_t
// using std::remove_pointer_t

#include <utility>
// using std::forward

#ifndef ITR_CONDENSATIONGRAPH_HPP
#define ITR_CONDENSATIONGRAPH_HPP

namespace iteratorrecognition {

template <typename GraphT>
using CondensationType =
    std::vector<typename llvm::GraphTraits<GraphT>::NodeRef>;

template <typename GraphT>
using CondensationVectorType = std::vector<CondensationType<GraphT>>;

template <typename GraphT>
using ConstCondensationVectorType = std::vector<const CondensationType<GraphT>>;

// forward declaration

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>,
          typename IGT = llvm::GraphTraits<llvm::Inverse<GraphT>>>
class CondensationGraph;

//

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>>
class CondensationGraphNode {
  friend class CondensationGraph<GraphT, GT>;

  using MemberNodeRef = typename GT::NodeRef;
  using EdgesContainerType = std::vector<CondensationGraphNode *>;

  std::vector<MemberNodeRef> Nodes;
  mutable EdgesContainerType OutEdges;
  mutable EdgesContainerType InEdges;

  template <typename IteratorT>
  explicit CondensationGraphNode(IteratorT Begin, IteratorT End) {
    static_assert(
        std::is_same<typename std::iterator_traits<IteratorT>::value_type,
                     MemberNodeRef>::value,
        "Iterator type cannot be dereferenced to the expected value!");

    std::copy(Begin, End, std::back_inserter(Nodes));
  }

public:
  using iterator = typename decltype(Nodes)::iterator;
  using const_iterator = typename decltype(Nodes)::const_iterator;

  using EdgesIteratorType = typename EdgesContainerType::iterator;
  using ConstEdgesIteratorType = typename EdgesContainerType::const_iterator;

  CondensationGraphNode() = delete;
  CondensationGraphNode(const CondensationGraphNode &) = delete;
  CondensationGraphNode &operator=(const CondensationGraphNode &) = delete;

  CondensationGraphNode(CondensationGraphNode &&) = default;

  iterator begin() { return Nodes.begin(); }
  iterator end() { return Nodes.end(); }

  const_iterator begin() const { return Nodes.begin(); }
  const_iterator end() const { return Nodes.end(); }

  decltype(auto) size() const { return Nodes.size(); }
  bool empty() const { return Nodes.empty(); }

  EdgesIteratorType edge_begin() { return OutEdges.begin(); }
  EdgesIteratorType edge_end() { return OutEdges.end(); }

  ConstEdgesIteratorType edge_begin() const { return OutEdges.begin(); }
  ConstEdgesIteratorType edge_end() const { return OutEdges.end(); }

  EdgesIteratorType inverse_edge_begin() { return InEdges.begin(); }
  EdgesIteratorType inverse_edge_end() { return InEdges.end(); }

  ConstEdgesIteratorType inverse_edge_begin() const { return InEdges.begin(); }
  ConstEdgesIteratorType inverse_edge_end() const { return InEdges.end(); }

  iterator find(const MemberNodeRef &Node) {
    return std::find(begin(), end(), Node);
  }

  const_iterator find(const MemberNodeRef &Node) const {
    return std::find(begin(), end(), Node);
  }

  template <typename V> bool contains(const V &Node) const {
    return std::find(begin(), end(), Node) != end();
  }
};

//
template <typename GraphT, typename GT, typename IGT> class CondensationGraph {
public:
  using MemberNodeRef = typename GT::NodeRef;
  using NodeType = CondensationGraphNode<GraphT, GT>;
  using NodeRef = NodeType *;
  using ConstNodeRef = const NodeType *;

private:
  std::vector<NodeType> Nodes;
  mutable NodeRef EntryNode;
  using NodeToCondensationMap = std::map<MemberNodeRef, ConstNodeRef>;

  template <typename TraversalGT = GT>
  void findCondensationExternalEdges(
      const NodeType &CondensationNode, const NodeToCondensationMap &Map,
      typename NodeType::EdgesContainerType &Dst) const {
    std::vector<MemberNodeRef> current, reachable, external;

    auto sue = [](auto &Vec) {
      std::sort(Vec.begin(), Vec.end());
      Vec.erase(std::unique(Vec.begin(), Vec.end()), Vec.end());
    };

    for (const auto &n : CondensationNode) {
      current.push_back(n);

      std::for_each(TraversalGT::child_begin(n), TraversalGT::child_end(n),
                    [&](const auto &e) { reachable.push_back(e); });
    }

    sue(current);
    sue(reachable);

    std::set_difference(reachable.begin(), reachable.end(), current.begin(),
                        current.end(), std::back_inserter(external));

    std::for_each(external.begin(), external.end(), [&](const auto &e) {
      Dst.push_back(const_cast<NodeRef>(Map.at(e)));
    });

    sue(Dst);
  }

  void populateCondensedEdges() const {
    NodeToCondensationMap n2c;

    for (const auto &cn : *this) {
      for (const auto &n : cn) {
        n2c.emplace(n, std::addressof(cn));

        if (!n->unit()) {
          EntryNode = const_cast<NodeRef>(std::addressof(cn));
        }
      }
    }

    auto sue = [](auto &Vec) {
      std::sort(Vec.begin(), Vec.end());
      Vec.erase(std::unique(Vec.begin(), Vec.end()), Vec.end());
    };

    for (const auto &cn : *this) {
      findCondensationExternalEdges(cn, n2c, cn.OutEdges);
      findCondensationExternalEdges<IGT>(cn, n2c, cn.InEdges);
    }
  }

public:
  // TODO the iterator dereferences to a reference type and not a pointer as
  // denoted by NodeRef, which might create confusion and incompatibilities
  using iterator = typename decltype(Nodes)::iterator;
  using const_iterator = typename decltype(Nodes)::const_iterator;

  CondensationGraph() = delete;
  CondensationGraph(const CondensationGraph &) = delete;
  CondensationGraph &operator=(const CondensationGraph &) = delete;

  template <typename IteratorT>
  explicit CondensationGraph(IteratorT Begin, IteratorT End) {
    for (auto &scc : llvm::make_range(Begin, End)) {
      Nodes.emplace_back(NodeType{std::begin(scc), std::end(scc)});
    }

    populateCondensedEdges();
  }

  iterator begin() { return Nodes.begin(); }
  iterator end() { return Nodes.end(); }

  const_iterator begin() const { return Nodes.begin(); }
  const_iterator end() const { return Nodes.end(); }

  NodeRef getEntryNode() { return EntryNode; }
  decltype(auto) size() const { return Nodes.size(); }
  bool empty() const { return Nodes.empty(); }

  iterator find(const MemberNodeRef &Node) {
    return std::find_if(begin(), end(),
                        [&](const auto &e) { return e.find(Node) != e.end(); });
  }

  const_iterator find(const MemberNodeRef &Node) const {
    return std::find_if(begin(), end(),
                        [&](const auto &e) { return e.find(Node) != e.end(); });
  }
};

//

} // namespace iteratorrecognition

namespace iteratorrecognition {

// generic base for easing the task of creating graph traits for graphs

template <typename GraphT> struct LLVMCondensationGraphTraitsHelperBase {
  using NodeRef = typename GraphT::NodeRef;
  using NodeType = std::remove_pointer_t<NodeRef>;

  using nodes_iterator = std::conditional_t<std::is_const<GraphT>::value,
                                            typename GraphT::const_iterator,
                                            typename GraphT::iterator>;
  using ChildIteratorType =
      std::conditional_t<std::is_const<GraphT>::value,
                         typename NodeType::ConstEdgesIteratorType,
                         typename NodeType::EdgesIteratorType>;
  using ChildEdgeIteratorType =
      std::conditional_t<std::is_const<GraphT>::value,
                         typename NodeType::ConstEdgesIteratorType,
                         typename NodeType::EdgesIteratorType>;

  using EdgeRef =
      typename std::iterator_traits<ChildEdgeIteratorType>::reference;

  static NodeRef getEntryNode(GraphT *G) { return G->getEntryNode(); }
  static unsigned size(GraphT *G) { return G->size(); }

  static NodeRef edge_dest(EdgeRef E) { return E; }

  static decltype(auto) nodes_begin(GraphT *G) { return G->begin(); }
  static decltype(auto) nodes_end(GraphT *G) { return G->end(); }

  static decltype(auto) nodes_begin(GraphT &G) { return G.begin(); }
  static decltype(auto) nodes_end(GraphT &G) { return G.end(); }

  template <typename T> static decltype(auto) nodes(T &&G) {
    return llvm::make_range(nodes_begin(std::forward<T>(G)),
                            nodes_end(std::forward<T>(G)));
  }

  static decltype(auto) child_begin(NodeRef N) { return N->edge_begin(); }
  static decltype(auto) child_end(NodeRef N) { return N->edge_end(); }

  static decltype(auto) children(NodeRef N) {
    return llvm::make_range(child_begin(N), child_end(N));
  }

  static decltype(auto) child_edge_begin(NodeRef N) { return N->edge_begin(); }
  static decltype(auto) child_edge_end(NodeRef N) { return N->edge_end(); }

  static decltype(auto) children_edges(NodeRef N) {
    return llvm::make_range(child_edge_begin(N), child_edge_end(N));
  }
};

template <typename GraphT> struct LLVMCondensationInverseGraphTraitsHelperBase {
  using NodeRef = typename GraphT::NodeRef;
  using NodeType = std::remove_pointer_t<NodeRef>;

  using nodes_iterator = std::conditional_t<std::is_const<GraphT>::value,
                                            typename GraphT::const_iterator,
                                            typename GraphT::iterator>;
  using ChildIteratorType =
      std::conditional_t<std::is_const<GraphT>::value,
                         typename NodeType::ConstEdgesIteratorType,
                         typename NodeType::EdgesIteratorType>;
  using ChildEdgeIteratorType =
      std::conditional_t<std::is_const<GraphT>::value,
                         typename NodeType::ConstEdgesIteratorType,
                         typename NodeType::EdgesIteratorType>;

  using EdgeRef =
      typename std::iterator_traits<ChildEdgeIteratorType>::reference;

  static NodeRef getEntryNode(GraphT *G) { return G->getEntryNode(); }
  static unsigned size(GraphT *G) { return G->size(); }

  static NodeRef edge_dest(EdgeRef E) { return E; }

  static decltype(auto) nodes_begin(GraphT *G) { return G->begin(); }
  static decltype(auto) nodes_end(GraphT *G) { return G->end(); }

  static decltype(auto) nodes_begin(GraphT &G) { return G.begin(); }
  static decltype(auto) nodes_end(GraphT &G) { return G.end(); }

  template <typename T> static decltype(auto) nodes(T &&G) {
    return llvm::make_range(nodes_begin(std::forward<T>(G)),
                            nodes_end(std::forward<T>(G)));
  }

  static decltype(auto) child_begin(NodeRef N) {
    return N->inverse_edge_begin();
  }
  static decltype(auto) child_end(NodeRef N) { return N->inverse_edge_end(); }

  static decltype(auto) children(NodeRef N) {
    return llvm::make_range(child_begin(N), child_end(N));
  }

  static decltype(auto) child_edge_begin(NodeRef N) {
    return N->inverse_edge_begin();
  }
  static decltype(auto) child_edge_end(NodeRef N) {
    return N->inverse_edge_end();
  }

  static decltype(auto) children_edges(NodeRef N) {
    return llvm::make_range(child_edge_begin(N), child_edge_end(N));
  }
};

} // namespace iteratorrecognition

#endif // header
