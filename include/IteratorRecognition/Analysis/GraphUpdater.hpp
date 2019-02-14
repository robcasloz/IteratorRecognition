//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Analysis/IteratorValueTracking.hpp"

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <vector>
// using std::vector

#include <memory>
// using std::unique_ptr
// using std::make_unique

namespace iteratorrecognition {

struct UpdateAction {
  virtual void update() = 0;
};

void ExecuteAction(UpdateAction &UA) { UA.update(); }

using ActionQueueT = std::vector<std::unique_ptr<UpdateAction>>;

//

template <typename T> class GraphUpdate : public UpdateAction {

  T &underlying() { return static_cast<T &>(*this); }
  T const &underlying() const { return static_cast<T const &>(*this); }

public:
  void update() override { this->underlying().performUpdate(); }
};

class GraphNoop : public GraphUpdate<GraphNoop> {
public:
  GraphNoop() = default;
  GraphNoop(const GraphNoop &) = default;

  void performUpdate() {}
};

template <typename T, typename InfoT>
class GraphEdgeConnect : public GraphUpdate<GraphEdgeConnect<T, InfoT>> {
  T Src, Dst;
  InfoT Info;

public:
  GraphEdgeConnect(T Src, T Dst, InfoT Info) : Src(Src), Dst(Dst), Info(Info) {}
  GraphEdgeConnect(const GraphEdgeConnect &) = default;

  void performUpdate() { Src->addDependentNode(Dst, Info); }
};

template <typename T>
class GraphEdgeDisconnect : public GraphUpdate<GraphEdgeDisconnect<T>> {
  T Src, Dst;

public:
  GraphEdgeDisconnect(T Src, T Dst) : Src(Src), Dst(Dst) {}
  GraphEdgeDisconnect(const GraphEdgeDisconnect &) = default;

  void performUpdate() { Src->removeDependentNode(Dst); }
};

template <typename T, typename InfoT>
class GraphEdgeInfoUpdate : public GraphUpdate<GraphEdgeInfoUpdate<T, InfoT>> {
  T Src, Dst;
  InfoT Info;

public:
  GraphEdgeInfoUpdate(T Src, T Dst, InfoT Info)
      : Src(Src), Dst(Dst), Info(Info) {}
  GraphEdgeInfoUpdate(const GraphEdgeInfoUpdate &) = default;

  void performUpdate() { Src->setEdgeInfo(Dst, Info); }
};

//

template <typename GraphT> class IteratorVarianceGraphUpdateGenerator {
  GraphT &G;
  IteratorVarianceAnalyzer &IVA;

  using GraphNodeT = typename GraphT::NodeType;
  using GraphNodeRefT = typename llvm::GraphTraits<GraphT *>::NodeRef;
  using UnitT = typename GraphNodeT::UnitType;
  using EdgeInfoT = typename GraphNodeT::EdgeInfoType::value_type;

public:
  IteratorVarianceGraphUpdateGenerator(GraphT &G, IteratorVarianceAnalyzer &IVA)
      : G(G), IVA(IVA) {}

  decltype(auto) create(UnitT Src, UnitT Dst) {
    auto srcIV = IVA.getOrInsertVariance(Src);
    auto dstIV = IVA.getOrInsertVariance(Dst);

    auto *srcNode = G.getNode(Src);
    auto *dstNode = G.getNode(Dst);

    std::unique_ptr<UpdateAction> doUpdate, undoUpdate;

    if (srcIV == IteratorVarianceValue::Unknown ||
        dstIV == IteratorVarianceValue::Unknown) {
      doUpdate = std::make_unique<GraphNoop>();
      undoUpdate = std::make_unique<GraphNoop>();
    } else if (srcIV == IteratorVarianceValue::Variant ||
               dstIV == IteratorVarianceValue::Variant) {
      if (auto infoOrEmpty = srcNode->getEdgeInfo(dstNode)) {
        if (auto info = *infoOrEmpty) {
          auto memInfo = info & pedigree::DO_Memory;
          auto newInfo = info;
          newInfo.reset(pedigree::DO_Memory);

          if (memInfo) {
            if (newInfo) {
              doUpdate = std::make_unique<
                  GraphEdgeInfoUpdate<GraphNodeRefT, EdgeInfoT>>(
                  srcNode, dstNode, newInfo);

              undoUpdate = std::make_unique<
                  GraphEdgeInfoUpdate<GraphNodeRefT, EdgeInfoT>>(
                  srcNode, dstNode, newInfo);
            } else {
              doUpdate = std::make_unique<GraphEdgeDisconnect<GraphNodeRefT>>(
                  srcNode, dstNode);

              undoUpdate =
                  std::make_unique<GraphEdgeConnect<GraphNodeRefT, EdgeInfoT>>(
                      srcNode, dstNode, memInfo);
            }
          }
        }
      }
    } else if (srcIV == IteratorVarianceValue::Invariant ||
               dstIV == IteratorVarianceValue::Invariant) {
      if (!srcNode->hasEdgeWith(dstNode)) {
        if (auto newInfo = determineHazard(Src, Dst)) {
          doUpdate =
              std::make_unique<GraphEdgeConnect<GraphNodeRefT, EdgeInfoT>>(
                  srcNode, dstNode, newInfo);

          undoUpdate = std::make_unique<GraphEdgeDisconnect<GraphNodeRefT>>(
              srcNode, dstNode);
        }
      }
    }

    return std::make_pair(std::move(doUpdate), std::move(undoUpdate));
  }
};

} // namespace iteratorrecognition

