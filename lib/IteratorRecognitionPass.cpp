//
//
//

#include "Config.hpp"

#include "Util.hpp"

#include "Debug.hpp"

#include "Analysis/Graphs/DependenceGraphs.hpp"

#include "Analysis/Passes/PDGraphPass.hpp"

#include "IteratorRecognitionPass.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/ADT/SCCIterator.h"
// using llvm::scc_begin
// using llvm::scc_end

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/EquivalenceClasses.h"
// using llvm::EquivalenceClasses

#include "llvm/ADT/iterator_range.h"
// llvm::make_range

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include <boost/iterator/iterator_facade.hpp>
// using boost::iterator_facade
// using boost::bidirectional_traversal_tag
// using boost::iterator_core_access

#include <memory>
// using std::unique_ptr

#include <vector>
// using std::vector

#include <iterator>
// using std::begin
// using std::end
// using std::distance

#include <type_traits>
// using std::is_trivially_copyable

#define DEBUG_TYPE "iterator-recognition"

// plugin registration for opt

char itr::IteratorRecognitionPass::ID = 0;
static llvm::RegisterPass<itr::IteratorRecognitionPass>
    X("itr", PRJ_CMDLINE_DESC("iterator recognition pass"), false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerIteratorRecognitionPass(const llvm::PassManagerBuilder &Builder,
                                llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::IteratorRecognitionPass());

  return;
}

static llvm::RegisterStandardPasses RegisterIteratorRecognitionPass(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    registerIteratorRecognitionPass);

//

static llvm::cl::OptionCategory
    IteratorRecognitionPassCategory("Iterator Recognition Pass",
                                    "Options for Iterator Recognition pass");

#if ITERATORRECOGNITION_DEBUG
static llvm::cl::opt<bool, true>
    Debug("itr-debug", llvm::cl::desc("debug iterator recognition pass"),
          llvm::cl::location(itr::debug::passDebugFlag),
          llvm::cl::cat(IteratorRecognitionPassCategory));

static llvm::cl::opt<LogLevel, true> DebugLevel(
    "itr-debug-level",
    llvm::cl::desc("debug level for Iterator Recognition pass"),
    llvm::cl::location(itr::debug::passLogLevel),
    llvm::cl::values(
        clEnumValN(LogLevel::Info, "Info", "informational messages"),
        clEnumValN(LogLevel::Notice, "Notice", "significant conditions"),
        clEnumValN(LogLevel::Warning, "Warning", "warning conditions"),
        clEnumValN(LogLevel::Error, "Error", "error conditions"),
        clEnumValN(LogLevel::Debug, "Debug", "debug messages")
// clang-format off
#if (LLVM_VERSION_MAJOR <= 3 && LLVM_VERSION_MINOR < 9)
        , clEnumValEnd
#endif
        // clang-format on
        ),
    llvm::cl::cat(IteratorRecognitionPassCategory));
#endif // ITERATORRECOGNITION_DEBUG

//

namespace itr {

using SCC_type = std::vector<llvm::GraphTraits<pedigree::PDGraph *>::NodeRef>;
using const_SCC_type = const SCC_type;
using SCCs_type = std::vector<const_SCC_type>;

template <typename NodeRefT, typename NodeT = std::remove_pointer_t<NodeRefT>>
struct CondensationGraph {
  static_assert(std::is_trivially_copyable<NodeRefT>::value,
                "NodeRef is not trivially copyable!");

  using NodeType = NodeT;
  using NodeRef = NodeRefT;

  llvm::EquivalenceClasses<NodeRef> Nodes;
  llvm::DenseMap<NodeRef, NodeRef> Edges;

  explicit CondensationGraph() = default;
  CondensationGraph(const CondensationGraph &G) = default;

  // TODO limit iterator type
  template <typename IteratorT>
  void addCondensedNode(IteratorT Begin, IteratorT End) {
    if (Begin == End)
      return;

    for (auto it = Begin; it != End; ++it)
      Nodes.unionSets(*Begin, *it);

    auto currentIt = Nodes.findLeader(*Begin);

    for (auto &n : llvm::make_range(Begin, End)) {
      for (const auto &e : n->nodes()) {
        auto adjacentIt = Nodes.findLeader(e);

        if (adjacentIt == Nodes.member_end() || adjacentIt == currentIt) {
          continue;
        }

        Edges.try_emplace(*currentIt, *adjacentIt);
      }
    }
  }

  decltype(auto) getEntryNode() const {
    return *(Nodes.member_begin(Nodes.begin()));
  }

  decltype(auto) size() const {
    return std::distance(Nodes.begin(), Nodes.end());
  }

  class CondensationGraphIterator
      : public boost::iterator_facade<CondensationGraphIterator, const NodeRef,
                                      boost::bidirectional_traversal_tag> {
  public:
    using base_iterator = typename decltype(Nodes)::iterator;

    const CondensationGraph *CurrentCG;
    base_iterator CurrentIt;

    CondensationGraphIterator() : CurrentCG(nullptr) {}
    CondensationGraphIterator(const CondensationGraph &CG)
        : CurrentCG(&CG), CurrentIt(CG.Nodes.begin()) {}

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

  using nodes_iterator = CondensationGraphIterator;

  decltype(auto) nodes_begin() const { return nodes_iterator(*this); }
  decltype(auto) nodes_end() const { return nodes_iterator(); }
};

} // namespace itr

namespace llvm {

template <>
struct llvm::GraphTraits<itr::CondensationGraph<itr::SCC_type::value_type>> {
  using GraphType = itr::CondensationGraph<itr::SCC_type::value_type>;
  using NodeType = GraphType::NodeType;

  using NodeRef = GraphType::NodeRef;

  static NodeRef getEntryNode(GraphType *G) { return G->getEntryNode(); }
  static unsigned size(GraphType *G) { return G->size(); }

  using ChildIteratorType = NodeType::nodes_iterator;
  static decltype(auto) child_begin(NodeRef G) { return G->nodes_begin(); }
  static decltype(auto) child_end(NodeRef G) { return G->nodes_end(); }

  using nodes_iterator = GraphType::nodes_iterator;
  static decltype(auto) nodes_begin(GraphType *G) { return G->nodes_begin(); }
  static decltype(auto) nodes_end(GraphType *G) { return G->nodes_end(); }
};

} // namespace llvm

namespace itr {

void IteratorRecognitionPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::LoopInfoWrapperPass>();
  AU.addRequired<pedigree::PDGraphPass>();

  AU.setPreservesAll();
}

void MapPDGSCCToLoop(const llvm::LoopInfo &LI, const pedigree::PDGraph &G,
                     SCCs_type &SCCs, llvm::DenseMap<int, llvm::Loop *> &Map) {
  for (auto i = 0; i < SCCs.size(); ++i) {
    llvm::Loop *loop = nullptr;

    for (auto j = 0; j < SCCs[i].size(); ++j) {
      loop = LI.getLoopFor((SCCs[i][j]->unit())->getParent());
      llvm::dbgs() << '-' << (SCCs[i][j]->unit())->getParent()->getName()
                   << '\n';

      if (loop) {
        break;
      }
    }

    Map.try_emplace(i, loop);
  }
}

bool IteratorRecognitionPass::runOnFunction(llvm::Function &CurFunc) {
  bool hasChanged = false;

  const auto *LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  if (LI->empty()) {
    return hasChanged;
  }

  pedigree::PDGraph &Graph{getAnalysis<pedigree::PDGraphPass>().getGraph()};
  SCCs_type SCCs;

  CondensationGraph<SCC_type::value_type> CG;

  for (auto scc = llvm::scc_begin(&Graph); !scc.isAtEnd(); ++scc) {
    SCCs.emplace_back(*scc);
    CG.addCondensedNode(std::begin(*scc), std::end(*scc));
  }

  auto b = llvm::GraphTraits<decltype(CG)>::nodes_begin(&CG);
  llvm::dbgs() << ">>>" << *((*b)->unit()) << '\n';

  llvm::DenseMap<int, llvm::Loop *> PDGSCCToLoop;
  MapPDGSCCToLoop(*LI, Graph, SCCs, PDGSCCToLoop);

  for (const auto &curLoop : *LI) {
    for (const auto *curBlock : curLoop->getBlocks()) {
      for (const auto &curInst : *curBlock) {
        llvm::dbgs() << curInst << '\n';
      }
    }
  }

  return hasChanged;
}

} // namespace itr
