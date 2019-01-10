//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include <vector>
// using std::vector

#include <map>
// using std::map

#include <iterator>
// using std::iterator_traits
// using std::begin
// using std::end

#include <memory>
// using std::unique_ptr
// using std::make_unique

namespace llvm {
class Instruction;
} // namespace llvm

namespace iteratorrecognition {

class ShadowDependenceGraph;

class ShadowDependenceGraphNode {
  friend class ShadowDependenceGraph;

private:
  using SelfType = ShadowDependenceGraphNode;
  using MemberNodeRef = llvm::Instruction *;

public:
  using IDType = uint64_t;

  explicit ShadowDependenceGraphNode(MemberNodeRef MemberNode) {
    Nodes.emplace_back(MemberNode);
  }

private:
  using EdgesContainerType = std::vector<SelfType *>;

  IDType ID;
  IDType ShadowID;
  std::vector<MemberNodeRef> Nodes;
  mutable EdgesContainerType OutEdges;
  mutable EdgesContainerType InEdges;

  template <typename IteratorT>
  explicit ShadowDependenceGraphNode(IteratorT Begin, IteratorT End) {
    std::copy(Begin, End, std::back_inserter(Nodes));
  }

public:
  using iterator = typename decltype(Nodes)::iterator;
  using const_iterator = typename decltype(Nodes)::const_iterator;

  using EdgesIteratorType = typename EdgesContainerType::iterator;
  using ConstEdgesIteratorType = typename EdgesContainerType::const_iterator;

  ShadowDependenceGraphNode() = delete;
  ShadowDependenceGraphNode(const SelfType &) = delete;
  SelfType &operator=(const SelfType &) = delete;

  ShadowDependenceGraphNode(SelfType &&) = default;

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

class ShadowDependenceGraph {
public:
  using MemberNodeRef = llvm::Instruction *;
  using NodeType = ShadowDependenceGraphNode;
  using NodeRef = NodeType *;
  using ConstNodeRef = const NodeType *;

private:
  std::vector<std::unique_ptr<NodeType>> Nodes;
  // mutable NodeRef EntryNode;

  using MemberNodeToNodeMap = std::map<MemberNodeRef, NodeRef>;
  MemberNodeToNodeMap ShadowNodesMap;

  template <typename IteratorT>
  void computeEdges(IteratorT Begin, IteratorT End) {
    using GT =
        llvm::GraphTraits<typename std::iterator_traits<IteratorT>::value_type>;

    for (IteratorT it = Begin, end = End; it != End; ++it) {
      auto &n = ShadowNodesMap[(*it)->unit()];
      for (const auto &e : GT::nodes(*it)) {
        auto &c = ShadowNodesMap[e->unit()];
        n->OutEdges.push_back(c);
        c->InEdges.push_back(n);
      }
    }
  }

public:
  ShadowDependenceGraph() = delete;
  ShadowDependenceGraph(const ShadowDependenceGraph &) = delete;
  ShadowDependenceGraph &operator=(const ShadowDependenceGraph &) = delete;

  template <typename IteratorT>
  explicit ShadowDependenceGraph(IteratorT Begin, IteratorT End) {
    using std::begin;
    using std::end;

    for (IteratorT it = Begin, end = End; it != End; ++it) {
      auto n{std::make_unique<NodeType>((*it)->unit())};
      ShadowNodesMap.emplace((*it)->unit(), n.get());
      Nodes.emplace_back(std::move(n));
    }

    computeEdges(Begin, End);
  }

  decltype(auto) size() const { return Nodes.size(); }
  bool empty() const { return Nodes.empty(); }
};

} // namespace iteratorrecognition

