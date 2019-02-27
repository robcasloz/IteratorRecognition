//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "Pedigree/Analysis/Info/EdgeInfo/BasicDependenceInfo.hpp"

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Instructions.h"
// using llvm::Argument
// using llvm::AllocaInst

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

#include <cassert>
// using assert

#define DEBUG_TYPE "iterator-recognition-sgraph"

namespace iteratorrecognition {

bool IsIteratorDependent(
    const llvm::Value *V,
    const llvm::SmallPtrSetImpl<llvm::Instruction *> &IteratorValues) {
  llvm::SmallVector<const llvm::Value *, 16> workList{V};

  while (!workList.empty()) {
    auto *v = workList.back();
    workList.pop_back();

    if (llvm::isa<llvm::Argument>(v) || llvm::isa<llvm::Constant>(v)) {
      continue;
    }

    auto *ptrInst = llvm::dyn_cast<llvm::Instruction>(v);
    if (!ptrInst) {
      continue;
    }

    if (IteratorValues.count(ptrInst)) {
      return true;
    }

    if (llvm::isa<llvm::AllocaInst>(ptrInst)) {
      continue;
    }

    workList.insert(workList.end(), ptrInst->op_begin(), ptrInst->op_end());
  }

  return false;
}

template <typename GraphT> class SDependenceGraph;

template <typename GraphT> class SDependenceGraphNode {
  friend class SDependenceGraph<GraphT>;

  using SelfType = SDependenceGraphNode;
  using UnitType = llvm::Instruction *;

  bool IsNextIteration;

public:
  explicit SDependenceGraphNode(UnitType U) : IsNextIteration(false) {
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

  iterator units_begin() { return begin(); }
  iterator units_end() { return end(); }

  const_iterator units_begin() const { return begin(); }
  const_iterator units_end() const { return end(); }

  decltype(auto) units() {
    return llvm::make_range(units_begin(), units_end());
  }

  decltype(auto) units() const {
    return llvm::make_range(units_begin(), units_end());
  }

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

  decltype(auto) inverse_edges() {
    return llvm::make_range(inverse_edges_begin(), inverse_edges_end());
  }

  decltype(auto) inverse_edges() const {
    return llvm::make_range(inverse_edges_begin(), inverse_edges_end());
  }

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

  GraphT &getGraph() { return OriginalGraph; }
  const GraphT &getGraph() const { return OriginalGraph; }

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

  decltype(auto) numInEdges() const {
    decltype(std::declval<NodeType>().numInEdges()) n{};
    std::for_each(std::begin(Nodes), std::end(Nodes),
                  [&n](const auto &e) { n += e.get()->numInEdges(); });
    return n;
  }

  void addNodeFor(UnitType U) {
    auto sn{std::make_unique<NodeType>(U)};
    UnitToNode.emplace(U, sn.get());
    Nodes.emplace_back(std::move(sn));
  }

  ConstNodeRef findNodeFor(UnitType U) const {
    for (auto it = nodes_begin(), end = nodes_end(); it != end; ++it) {
      auto *n = *it;
      auto found = std::find(n->units_begin(), n->units_end(), U);

      if (found != n->Units.end()) {
        return n;
      }
    }

    return nullptr;
  }

  void removeNodeFor(UnitType U) {
    for (auto it = Nodes.begin(), end = Nodes.end(); it != end; ++it) {
      auto &n = *it;
      auto found = std::find(n->Units.begin(), n->Units.end(), U);

      if (found != n->Units.end()) {
        UnitToNode.erase(*found);
        n->Units.erase(found);

        if (n->Units.size() == 0) {
          Nodes.erase(it);
        }

        return;
      }
    }
  }

  // template <typename PredT> void computeNodesIf(PredT Pred) {
  // for (const auto &n :
  // llvm::make_filter_range(GT::nodes(&OriginalGraph), Pred)) {
  // addNodeFor(n->unit());
  //}
  //}

  template <typename PredT>
  void computeNodes(PredT Pred = [](const auto &e) { return true; }) {
    for (const auto &n : GT::nodes(&OriginalGraph)) {
      if (Pred(n)) {
        addNodeFor(n->unit());
      }
    }
  }

  void computeEdges() {
    for (const auto &n : GT::nodes(&OriginalGraph)) {
      if (!UnitToNode.count(n->unit())) {
        continue;
      }

      auto &sn = UnitToNode[n->unit()];
      for (const auto &e : GT::children(n)) {
        if (!UnitToNode.count(e->unit())) {
          continue;
        }

        auto &c = UnitToNode[e->unit()];
        sn->OutEdges.push_back(c);
        c->InEdges.push_back(sn);
      }
    }
  }

  void computeNextIterationNodes() {
    decltype(Nodes) niNodes;
    for (auto &n0 : Nodes) {
      for (auto &u : n0->units()) {
        auto n1{std::make_unique<NodeType>(u)};
        n1->setNextIteration(true);
        Iterations.insert(
            typename NodeToNodeBimap::value_type(n0.get(), n1.get()));
        niNodes.emplace_back(std::move(n1));
      }
    }

    for (auto &n1 : niNodes) {
      Nodes.emplace_back(std::move(n1));
    }
  }

  void computeNextIterationEdges() {
    for (auto &n1 : Nodes) {
      if (!n1->isNextIteration()) {
        continue;
      }

      auto &n0 = Iterations.template by<iteration1>().at(n1.get());

      for (auto &en0 : n0->edges()) {
        auto &en1 = Iterations.template by<iteration0>().at(en0);
        n1->OutEdges.push_back(en1);
        en1->InEdges.push_back(n0);
      }
    }
  }

  void computeCrossIterationEdges(
      const llvm::SmallPtrSetImpl<llvm::Instruction *> &IteratorValues) {
    std::map<NodeRef, std::vector<NodeRef>> ciEdges;

    for (auto *n0 : nodes()) {
      if (n0->isNextIteration()) {
        continue;
      }

      std::set<NodeRef> nodeSources;

      for (auto *unitSrc : n0->units()) {
        auto *depSrc = OriginalGraph.getNode(unitSrc);
        assert(depSrc && "Pointer is null!");

        std::set<UnitType> unitDestinations;

        for (auto &e : n0->edges()) {
          for (auto *unitDst : *e) {
            auto *depDst = OriginalGraph.getNode(unitDst);
            assert(depDst && "Pointer is null!");

            auto info = depSrc->getEdgeInfo(depDst);

            if (info.value().has(pedigree::DO_Memory)) {
              if (IsIteratorDependent(unitSrc, IteratorValues)) {
                llvm::dbgs() << "test it dep: " << *unitSrc << "\n";
              }

              unitDestinations.insert(depDst->unit());
            }
          }

          for (auto &e : unitDestinations) {
            nodeSources.insert(UnitToNode[e]);
          }
        }
      }

      NodeRef n1 = Iterations.template by<iteration0>().at(n0);

      ciEdges.insert({n1, {}});
      for (auto &e : nodeSources) {
        ciEdges[n1].emplace_back(e);
      }
    }

    for (auto &e : ciEdges) {
      auto *n1 = e.first;
      for (auto *n0 : e.second) {
        n0->OutEdges.push_back(n1);
        n1->InEdges.push_back(n0);
      }
    }
  }
};

} // namespace iteratorrecognition

