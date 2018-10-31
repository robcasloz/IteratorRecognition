//
//
//

#include "Config.hpp"

#include "Debug.hpp"

#include "Analysis/Graphs/PDGraph.hpp"

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
// using std::conditional_t
// using std::is_pointer
// using std::remove_pointer_t
// using std::remove_reference_t
// using std::is_class

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

// forward declaration

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>>
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

public:
  using iterator = typename decltype(Nodes)::iterator;
  using const_iterator = typename decltype(Nodes)::const_iterator;

  using EdgesIteratorType = typename EdgesContainerType::iterator;

  CondensationGraphNode() = delete;
  CondensationGraphNode(const CondensationGraphNode &) = delete;
  CondensationGraphNode &operator=(const CondensationGraphNode &) = delete;

  CondensationGraphNode(CondensationGraphNode &&) = default;

  // TODO consider making this ctor private
  template <typename IteratorT>
  explicit CondensationGraphNode(IteratorT Begin, IteratorT End) {
    static_assert(
        std::is_same<typename std::iterator_traits<IteratorT>::value_type,
                     MemberNodeRef>::value,
        "Iterator type cannot be dereferenced to the expected value!");

    std::copy(Begin, End, std::back_inserter(Nodes));
  }

  iterator begin() { return Nodes.begin(); }
  iterator end() { return Nodes.end(); }

  const_iterator begin() const { return Nodes.begin(); }
  const_iterator end() const { return Nodes.end(); }

  decltype(auto) size() const { return Nodes.size(); }
  bool empty() const { return Nodes.empty(); }

  EdgesIteratorType edge_begin() { return OutEdges.begin(); }
  EdgesIteratorType edge_end() { return OutEdges.end(); }

  EdgesIteratorType edge_begin() const { return OutEdges.begin(); }
  EdgesIteratorType edge_end() const { return OutEdges.end(); }

  EdgesIteratorType inverse_edge_begin() { return InEdges.begin(); }
  EdgesIteratorType inverse_edge_end() { return InEdges.end(); }

  EdgesIteratorType inverse_edge_begin() const { return InEdges.begin(); }
  EdgesIteratorType inverse_edge_end() const { return InEdges.end(); }

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

template <typename GraphT, typename GT> class CondensationGraph {
public:
  using MemberNodeRef = typename GT::NodeRef;
  using NodeType = CondensationGraphNode<GraphT, GT>;
  using NodeRef = NodeType *;
  using ConstNodeRef = const NodeType *;

private:
  std::vector<NodeType> Nodes;
  NodeRef EntryNode;

  void populateCondensedEdges() const {
    std::map<MemberNodeRef, ConstNodeRef> nodeToCondensation;

    for (const auto &cn : *this) {
      for (const auto &n : cn) {
        nodeToCondensation.emplace(n, std::addressof(cn));
      }
    }

    for (const auto &cn : *this) {
      llvm::DenseSet<MemberNodeRef> curNodes, outNodes, inNodes;
      std::vector<MemberNodeRef> inNodesDiff, outNodesDiff;

      for (const auto &n : cn) {
        curNodes.insert(n);

        std::for_each(n->nodes_begin(), n->nodes_end(),
                      [&](const auto &e) { outNodes.insert(e); });

        std::for_each(n->inverse_nodes_begin(), n->inverse_nodes_end(),
                      [&](const auto &e) { inNodes.insert(e); });
      }

      std::set_difference(outNodes.begin(), outNodes.end(), curNodes.begin(),
                          curNodes.end(), std::back_inserter(outNodesDiff));
      std::set_difference(inNodes.begin(), inNodes.end(), curNodes.begin(),
                          curNodes.end(), std::back_inserter(inNodesDiff));

      std::sort(outNodesDiff.begin(), outNodesDiff.end());
      std::sort(inNodesDiff.begin(), inNodesDiff.end());

      auto outLast = std::unique(outNodesDiff.begin(), outNodesDiff.end());
      outNodesDiff.erase(outLast, outNodesDiff.end());

      auto inLast = std::unique(inNodesDiff.begin(), inNodesDiff.end());
      inNodesDiff.erase(inLast, inNodesDiff.end());

      std::for_each(outNodesDiff.begin(), outNodesDiff.end(),
                    [&](const auto &e) {
                      cn.OutEdges.push_back(
                          const_cast<NodeRef>(nodeToCondensation.at(e)));
                    });

      std::for_each(inNodesDiff.begin(), inNodesDiff.end(), [&](const auto &e) {
        cn.InEdges.push_back(const_cast<NodeRef>(nodeToCondensation.at(e)));
      });
    }
  }

public:
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
};

//

} // namespace itr

namespace itr {

// generic base for easing the task of creating graph traits for graphs

// TODO add const variant of traits helper

template <typename GraphT> struct LLVMCondensationGraphTraitsHelperBase {
  using NodeRef = typename GraphT::NodeRef;
  using NodeType =
      typename std::conditional_t<std::is_pointer<NodeRef>::value,
                                  std::remove_pointer_t<NodeRef>,
                                  std::remove_reference_t<NodeRef>>;

  // TODO what if NodeRef is a reference_wrapper?
  static_assert(std::is_class<NodeType>::value,
                "NodeType is not a class type!");

  using nodes_iterator = typename GraphT::iterator;
  using ChildIteratorType = typename NodeType::EdgesIteratorType;
  using ChildEdgeIteratorType = typename NodeType::EdgesIteratorType;

  using EdgeRef =
      typename std::iterator_traits<ChildEdgeIteratorType>::reference;

  static NodeRef getEntryNode(GraphT *G) { return G->getEntryNode(); }
  static unsigned size(GraphT *G) { return G->size(); }

  static NodeRef edge_dest(EdgeRef E) { return E; }

  static decltype(auto) nodes_begin(GraphT *G) { return G->begin(); }
  static decltype(auto) nodes_end(GraphT *G) { return G->end(); }

  static decltype(auto) child_begin(NodeRef N) { return N->edge_begin(); }
  static decltype(auto) child_end(NodeRef N) { return N->edge_end(); }

  static decltype(auto) child_edge_begin(NodeRef N) { return N->edge_begin(); }
  static decltype(auto) child_edge_end(NodeRef N) { return N->edge_end(); }
};

template <typename GraphT> struct LLVMCondensationInverseGraphTraitsHelperBase {
  using NodeRef = typename GraphT::NodeRef;
  using NodeType =
      typename std::conditional_t<std::is_pointer<NodeRef>::value,
                                  std::remove_pointer_t<NodeRef>,
                                  std::remove_reference_t<NodeRef>>;

  // TODO what if NodeRef is a reference_wrapper?
  static_assert(std::is_class<NodeType>::value,
                "NodeType is not a class type!");

  using nodes_iterator = typename GraphT::iterator;
  using ChildIteratorType = typename NodeType::EdgesIteratorType;
  using ChildEdgeIteratorType = typename NodeType::EdgesIteratorType;

  using EdgeRef =
      typename std::iterator_traits<ChildEdgeIteratorType>::reference;

  static NodeRef getEntryNode(GraphT *G) { return G->getEntryNode(); }
  static unsigned size(GraphT *G) { return G->size(); }

  static NodeRef edge_dest(EdgeRef E) { return E; }

  static decltype(auto) nodes_begin(GraphT *G) { return G->begin(); }
  static decltype(auto) nodes_end(GraphT *G) { return G->end(); }

  static decltype(auto) child_begin(NodeRef N) {
    return N->inverse_edge_begin();
  }
  static decltype(auto) child_end(NodeRef N) { return N->inverse_edge_end(); }

  static decltype(auto) child_edge_begin(NodeRef N) {
    return N->inverse_edge_begin();
  }
  static decltype(auto) child_edge_end(NodeRef N) {
    return N->inverse_edge_end();
  }
};

} // namespace itr

#endif // header
