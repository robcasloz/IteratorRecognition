//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "llvm/ADT/STLExtras.h"
// using llvm::make_filter_range

#include <vector>
// using std::vector

#include <map>
// using std::map

#include <memory>
// using std::unique_ptr
// using std::make_unique

namespace llvm {
class Instruction;
} // namespace llvm

namespace iteratorrecognition {

template <typename GraphT> class SDependenceGraph;

template <typename GraphT> class SDependenceGraphNode {
  friend class SDependenceGraph<GraphT>;

  using SelfType = SDependenceGraphNode;
  using MemberNodeRef = llvm::Instruction *;

  SDependenceGraph<GraphT> *ContainingGraph;

public:
  explicit SDependenceGraphNode(MemberNodeRef MemberNode)
      : ContainingGraph(nullptr) {
    Nodes.emplace_back(MemberNode);
  }

private:
  using EdgesContainerType = std::vector<SelfType *>;

  std::vector<MemberNodeRef> Nodes;
  mutable EdgesContainerType OutEdges;
  mutable EdgesContainerType InEdges;

  template <typename IteratorT>
  explicit SDependenceGraphNode(IteratorT Begin, IteratorT End) {
    std::copy(Begin, End, std::back_inserter(Nodes));
  }

public:
  using iterator = typename decltype(Nodes)::iterator;
  using const_iterator = typename decltype(Nodes)::const_iterator;

  using EdgesIteratorType = typename EdgesContainerType::iterator;
  using ConstEdgesIteratorType = typename EdgesContainerType::const_iterator;

  SDependenceGraphNode() = delete;
  SDependenceGraphNode(const SelfType &) = delete;
  SelfType &operator=(const SelfType &) = delete;

  SDependenceGraphNode(SelfType &&) = default;

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

template <typename GraphT> class SDependenceGraph {
  using GT = llvm::GraphTraits<GraphT *>;

  GraphT &OriginalGraph;

public:
  using MemberNodeRef = llvm::Instruction *;
  using NodeType = SDependenceGraphNode<GraphT>;
  using NodeRef = NodeType *;
  using ConstNodeRef = const NodeType *;

private:
  std::vector<std::unique_ptr<NodeType>> Nodes;

  using MemberNodeToNodeMap = std::map<MemberNodeRef, NodeRef>;
  MemberNodeToNodeMap SNodesMap;

public:
  SDependenceGraph() = delete;
  SDependenceGraph(const SDependenceGraph &) = delete;
  SDependenceGraph &operator=(const SDependenceGraph &) = delete;

  explicit SDependenceGraph(GraphT &G) : OriginalGraph(G) {}

  void computeNodes() {
    for (const auto &n : GT::nodes(&OriginalGraph)) {
      auto sn{std::make_unique<NodeType>(n->unit())};
      SNodesMap.emplace(n->unit(), sn.get());
      Nodes.emplace_back(std::move(sn));
    }
  }

  template <typename PredT> void computeNodesIf(PredT &&Pred) {
    for (const auto &n :
         llvm::make_filter_range(GT::nodes(&OriginalGraph), Pred)) {
      auto sn{std::make_unique<NodeType>(n->unit())};
      SNodesMap.emplace(n->unit(), sn.get());
      Nodes.emplace_back(std::move(sn));
    }
  }

  void computeEdges() {
    for (const auto &n : GT::nodes(&OriginalGraph)) {
      auto &sn = SNodesMap[n->unit()];
      for (const auto &e : GT::children(n)) {
        auto &c = SNodesMap[e->unit()];
        sn->OutEdges.push_back(c);
        c->InEdges.push_back(sn);
      }
    }
  }

  decltype(auto) size() const { return Nodes.size(); }
  bool empty() const { return Nodes.empty(); }
};

} // namespace iteratorrecognition

