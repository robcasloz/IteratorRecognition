//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "Pedigree/Analysis/Info/EdgeInfo/BasicDependenceInfo.hpp"

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range

#include "llvm/ADT/STLExtras.h"
// using llvm::mapped_iterator
// using llvm::make_filter_range

#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/Debug.h"
// using llvm::dbgs

#include "boost/bimap.hpp"
// using boost::bimap

#include <vector>
// using std::vector

#include <map>
// using std::map

#include <algorithm>
// using std::for_each

#include <memory>
// using std::unique_ptr
// using std::make_unique

#include <utility>
// using std::declval

#define DEBUG_TYPE "iterator-recognition-sgraph"

namespace llvm {
class Instruction;
} // namespace llvm

namespace iteratorrecognition {

template <typename GraphT> class SDependenceGraph;

template <typename GraphT> class SDependenceGraphNode {
  friend class SDependenceGraph<GraphT>;

  using SelfType = SDependenceGraphNode;
  using UnitType = llvm::Instruction *;

  SDependenceGraph<GraphT> *ContainingGraph;
  bool IsNextIteration;

public:
  explicit SDependenceGraphNode(UnitType U)
      : ContainingGraph(nullptr), IsNextIteration(false) {
    Units.emplace_back(U);
  }

private:
  using EdgesContainerType = std::vector<SelfType *>;

  std::vector<UnitType> Units;
  mutable EdgesContainerType OutEdges;
  mutable EdgesContainerType InEdges;

  template <typename IteratorT>
  explicit SDependenceGraphNode(IteratorT Begin, IteratorT End) {
    std::copy(Begin, End, std::back_inserter(Units));
  }

public:
  using iterator = typename decltype(Units)::iterator;
  using const_iterator = typename decltype(Units)::const_iterator;

  using EdgesIteratorType = typename EdgesContainerType::iterator;
  using ConstEdgesIteratorType = typename EdgesContainerType::const_iterator;

  SDependenceGraphNode() = delete;
  SDependenceGraphNode(const SelfType &) = delete;
  SelfType &operator=(const SelfType &) = delete;

  SDependenceGraphNode(SelfType &&) = default;

  bool isNextIteration() const { return IsNextIteration; }
  void setNextIteration(bool Val) { IsNextIteration = Val; }

  iterator begin() { return Units.begin(); }
  iterator end() { return Units.end(); }

  const_iterator begin() const { return Units.begin(); }
  const_iterator end() const { return Units.end(); }

  decltype(auto) size() const { return Units.size(); }
  bool empty() const { return Units.empty(); }

  decltype(auto) numOutEdges() const { return OutEdges.size(); }
  decltype(auto) numInEdges() const { return InEdges.size(); }

  EdgesIteratorType edges_begin() { return OutEdges.begin(); }
  EdgesIteratorType edges_end() { return OutEdges.end(); }

  ConstEdgesIteratorType edges_begin() const { return OutEdges.begin(); }
  ConstEdgesIteratorType edges_end() const { return OutEdges.end(); }

  decltype(auto) edges() {
    return llvm::make_range(edges_begin(), edges_end());
  }

  decltype(auto) edges() const {
    return llvm::make_range(edges_begin(), edges_end());
  }

  EdgesIteratorType inverse_edges_begin() { return InEdges.begin(); }
  EdgesIteratorType inverse_edges_end() { return InEdges.end(); }

  ConstEdgesIteratorType inverse_edges_begin() const { return InEdges.begin(); }
  ConstEdgesIteratorType inverse_edges_end() const { return InEdges.end(); }

  iterator find(const UnitType &Node) {
    return std::find(begin(), end(), Node);
  }

  const_iterator find(const UnitType &Node) const {
    return std::find(begin(), end(), Node);
  }

  template <typename V> bool contains(const V &Node) const {
    return std::find(begin(), end(), Node) != end();
  }
};

//

struct iteration0 {};
struct iteration1 {};

template <typename GraphT> class SDependenceGraph {
  using GT = llvm::GraphTraits<GraphT *>;

  GraphT &OriginalGraph;

public:
  using UnitType = llvm::Instruction *;
  using NodeType = SDependenceGraphNode<GraphT>;
  using NodeRef = NodeType *;
  using ConstNodeRef = const NodeType *;

private:
  std::vector<std::unique_ptr<NodeType>> Nodes;

  using NodeToNodeBimap =
      boost::bimap<boost::bimaps::tagged<NodeRef, iteration0>,
                   boost::bimaps::tagged<NodeRef, iteration1>>;
  NodeToNodeBimap Iterations;

  using UnitToNodeMap = std::map<UnitType, NodeRef>;
  UnitToNodeMap UnitToNode;

  static bool is_next_iteration_node(NodeRef N) { return N->isNextIteration(); }

  static bool is_not_next_iteration_node(ConstNodeRef N) {
    return !N->isNextIteration();
  }

public:
  using nodes_iterator = llvm::mapped_iterator<
      typename decltype(Nodes)::iterator,
      std::function<NodeRef(typename decltype(Nodes)::value_type &)>>;

  using nodes_const_iterator =
      llvm::mapped_iterator<typename decltype(Nodes)::const_iterator,
                            std::function<ConstNodeRef(
                                const typename decltype(Nodes)::value_type &)>>;

  SDependenceGraph() = delete;
  SDependenceGraph(const SDependenceGraph &) = delete;
  SDependenceGraph &operator=(const SDependenceGraph &) = delete;

  decltype(auto) size() const { return Nodes.size(); }
  bool empty() const { return Nodes.empty(); }

  static NodeRef nodes_iterator_map(typename decltype(Nodes)::value_type &P) {
    return P.get();
  }

  static ConstNodeRef
  nodes_const_iterator_map(const typename decltype(Nodes)::value_type &P) {
    return P.get();
  }

  explicit SDependenceGraph(GraphT &G) : OriginalGraph(G) {}

  decltype(auto) begin() {
    return nodes_iterator(Nodes.begin(), nodes_iterator_map);
  }

  decltype(auto) end() {
    return nodes_iterator(Nodes.end(), nodes_iterator_map);
  }

  decltype(auto) begin() const {
    return nodes_const_iterator(Nodes.begin(), nodes_const_iterator_map);
  }

  decltype(auto) end() const {
    return nodes_const_iterator(Nodes.end(), nodes_const_iterator_map);
  }

  decltype(auto) nodes_begin() { return begin(); }
  decltype(auto) nodes_end() { return end(); }

  decltype(auto) nodes_begin() const { return begin(); }
  decltype(auto) nodes_end() const { return end(); }

  decltype(auto) nodes() { return llvm::make_range(begin(), end()); }
  decltype(auto) nodes() const { return llvm::make_range(begin(), end()); }

  // TODO these range methods require a patch for llvm::iterator_range
  decltype(auto) iteration0_nodes() {
    return llvm::make_filter_range(nodes(), is_not_next_iteration_node);
  }

  decltype(auto) iteration0_nodes() const {
    return llvm::make_filter_range(nodes(), is_not_next_iteration_node);
  }

  decltype(auto) iteration1_nodes() {
    return llvm::make_filter_range(nodes(), is_next_iteration_node);
  }

  decltype(auto) iteration1_nodes() const {
    return llvm::make_filter_range(nodes(), is_next_iteration_node);
  }

  decltype(auto) numOutEdges() const {
    decltype(std::declval<NodeType>().numOutEdges()) n{};
    std::for_each(std::begin(Nodes), std::end(Nodes),
                  [&n](const auto &e) { n += e.get()->numOutEdges(); });
    return n;
  }

  void addNodeFor(UnitType I) {
    auto sn{std::make_unique<NodeType>(I)};
    (*sn).ContainingGraph = this;
    UnitToNode.emplace(I, sn.get());
    Nodes.emplace_back(std::move(sn));
  }

  void removeNodeFor(UnitType I) {
    for (auto it = Nodes.begin(), end = Nodes.end(); it != end; ++it) {
      auto &n = *it;
      auto found = std::find(n->Units.begin(), n->Units.end(), I);

      if (found != n->Units.end()) {
        n->Units.erase(found);

        if (n->Units.size() == 0) {
          Nodes.erase(it);
        }

        return;
      }
    }
  }

  void computeNodes() {
    for (const auto &n : GT::nodes(&OriginalGraph)) {
      addNodeFor(n->unit());
    }
  }

  template <typename PredT> void computeNodesIf(PredT &&Pred) {
    for (const auto &n :
         llvm::make_filter_range(GT::nodes(&OriginalGraph), Pred)) {
      addNodeFor(n->unit());
    }
  }

  void computeEdges() {
    for (const auto &n : GT::nodes(&OriginalGraph)) {
      auto &sn = UnitToNode[n->unit()];
      for (const auto &e : GT::children(n)) {
        auto &c = UnitToNode[e->unit()];
        sn->OutEdges.push_back(c);
        c->InEdges.push_back(sn);
      }
    }
  }

  void computeNextIterationNodes() {
    decltype(Nodes) niNodes;
    for (auto &n : Nodes) {
      for (auto &mn : n->Units) {
        auto sn{std::make_unique<NodeType>(mn)};
        (*sn).ContainingGraph = this;
        sn->setNextIteration(true);
        Iterations.insert(
            typename NodeToNodeBimap::value_type(n.get(), sn.get()));
        niNodes.emplace_back(std::move(sn));
      }
    }

    for (auto &n : niNodes) {
      Nodes.emplace_back(std::move(n));
    }
  }

  void computeNextIterationEdges() {
    for (const auto &sn : Nodes) {
      if (!sn->isNextIteration()) {
        continue;
      }

      auto &n = Iterations.template by<iteration1>().at(sn.get());

      for (auto &mn : n->Units) {
        auto &nn = UnitToNode[mn];
        auto &snn = Iterations.template by<iteration0>().at(nn);
        sn->OutEdges.push_back(snn);
        snn->InEdges.push_back(sn.get());
      }
    }
  }

  void computeCrossIterationEdges() {
    for (auto &n : Nodes) {
      if (n->isNextIteration()) {
        continue;
      }

      std::set<UnitType> unitDestinations;

      const auto &depSrc = *OriginalGraph.getNode(*(n->begin()));

      for (auto &e : n->edges()) {
        const auto &depDst = *OriginalGraph.getNode(*(e->begin()));

        auto info = depSrc->getEdgeInfo(depDst);

        if (info->origins & pedigree::DependenceOrigin::Memory) {
          unitDestinations.insert(depDst->unit());
        }
      }

      std::set<NodeRef> nodeSources;

      for (auto &e : unitDestinations) {
        nodeSources.insert(UnitToNode[e]);
      }

      NodeRef nodeDest = Iterations.template by<iteration0>().at(n.get());

      for (auto &e : nodeSources) {
        nodeDest->OutEdges.push_back(e);
        e->InEdges.push_back(nodeDest);
      }
    }
  }
};

} // namespace iteratorrecognition

