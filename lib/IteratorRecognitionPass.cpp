//
//
//

#include "Config.hpp"

#include "Util.hpp"

#include "Debug.hpp"

#include "Analysis/Graphs/DependenceGraphs.hpp"

#include "Analysis/Passes/PDGraphPass.hpp"

#include "IteratorRecognitionPass.hpp"

#include "CondensationGraph.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/ADT/SCCIterator.h"
// using llvm::scc_begin
// using llvm::scc_end

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "llvm/Support/JSON.h"
// using json::Value
// using json::Object
// using json::Array

#include "llvm/Support/FileSystem.h"
// using llvm::sys::fs::F_Text

#include "llvm/Support/ToolOutputFile.h"
// using llvm::ToolOutputFile

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream
// using llvm::raw_string_ostream

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <string>
// using std::string

#include <algorithm>
// using std::any_of
// using std::for_each

#include <iterator>
// using std::begin
// using std::end

#include <utility>
// using std::move

#include <system_error>
// using std::error_code

#include <cassert>
// using assert

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

static llvm::cl::opt<bool> SanityCheck(
    "itr-sanity-check", llvm::cl::desc("apply various sanity checks"),
    llvm::cl::init(false), llvm::cl::cat(IteratorRecognitionPassCategory));

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

using PDGCondensationType = CondensationType<pedigree::PDGraph *>;

using ConstPDGCondensationVector =
    ConstCondensationVectorType<pedigree::PDGraph *>;
} // namespace itr

namespace llvm {

template <>
struct GraphTraits<itr::CondensationGraph<pedigree::PDGraph *>>
    : public itr::LLVMCondensationGraphTraitsHelperBase<
          itr::CondensationGraph<pedigree::PDGraph *>> {};

template <>
struct GraphTraits<Inverse<itr::CondensationGraph<pedigree::PDGraph *>>>
    : public itr::LLVMCondensationInverseGraphTraitsHelperBase<
          itr::CondensationGraph<pedigree::PDGraph *>> {};

} // namespace llvm

namespace itr {

void IteratorRecognitionPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::LoopInfoWrapperPass>();
  AU.addRequired<pedigree::PDGraphPass>();

  AU.setPreservesAll();
}

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>>
void CheckCondensationToLoopMapping(GraphT &G, const llvm::LoopInfo &LI) {
  llvm::json::Object root;
  llvm::json::Array condensations;

  assert(LI.begin() != LI.end() && "No loops found!");
  const auto &functionName = (*LI.begin())->getHeader()->getParent()->getName();

  for (auto &n : G.nodes()) {
    if (!n->unit()) {
      continue;
    }

    llvm::DenseSet<llvm::Loop *> loops;

    std::for_each(G.scc_members_begin(n), G.scc_members_end(),
                  [&](const auto &m) {
                    if (m->unit()) {
                      loops.insert(LI.getLoopFor(m->unit()->getParent()));
                    }
                  });
    loops.erase(nullptr);

    llvm::json::Object mapping;
    std::string outs;
    llvm::raw_string_ostream ss(outs);
    ss << *n->unit();
    mapping["condensation"] = ss.str();
    mapping["loops"] = loops.size();

    condensations.push_back(std::move(mapping));
  }

  root["condensations"] = std::move(condensations);

  std::error_code ec;
  llvm::ToolOutputFile of("itr_" + functionName.str() + ".json", ec,
                          llvm::sys::fs::F_Text);

  if (ec) {
    llvm::errs() << "error opening file for writing!\n";
    of.os().clear_error();
  }

  of.os() << llvm::json::Value(std::move(root));
  of.os().close();

  if (!of.os().has_error()) {
    of.keep();
  }
}

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>>
void MapCondensationToLoop(
    GraphT &G, const llvm::LoopInfo &LI,
    llvm::DenseMap<typename GT::NodeRef, llvm::Loop *> &Map) {
  for (auto &n : G.nodes()) {
    llvm::Loop *loop;

    if (std::any_of(G.scc_members_begin(n), G.scc_members_end(),
                    [&](const auto &m) {
                      loop = nullptr;
                      return m->unit() &&
                             (loop = LI.getLoopFor(m->unit()->getParent()));
                    })) {
      Map.try_emplace(n, loop);
    }
  }
}

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>,
          typename IGT = llvm::GraphTraits<llvm::Inverse<GraphT>>>
void RecognizeIterator(GraphT &G) {}

bool IteratorRecognitionPass::runOnFunction(llvm::Function &CurFunc) {
  bool hasChanged = false;

  const auto *LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  if (LI->empty()) {
    return hasChanged;
  }

  pedigree::PDGraph &Graph{getAnalysis<pedigree::PDGraphPass>().getGraph()};
  ConstPDGCondensationVector CV;

  Graph.connectRootNode();

  llvm::dbgs() << "+++ " << Graph.numOutEdges() << '\n';

  CondensationGraph<pedigree::PDGraph *> CG{llvm::scc_begin(&Graph),
                                            llvm::scc_end(&Graph)};

  for (auto &n : CG.nodes()) {
    if (n->unit()) {
      llvm::dbgs() << ">>>" << *(n->unit()) << '\n';
      for (auto &m : CG.scc_members(n)) {
        if (m->unit())
          llvm::dbgs() << ">>>>>" << *(m->unit()) << '\n';
      }
    }
  }

  llvm::dbgs() << "condensations found: " << CG.size() << '\n';

  if (SanityCheck) {
    CheckCondensationToLoopMapping(CG, *LI);
  }

  llvm::DenseMap<typename llvm::GraphTraits<decltype(CG)>::NodeRef,
                 llvm::Loop *>
      CondensationToLoop;
  MapCondensationToLoop(CG, *LI, CondensationToLoop);

  llvm::dbgs() << "mappings: " << CondensationToLoop.size() << '\n';
  for (auto &e : CondensationToLoop) {
    llvm::dbgs() << e.getFirst()->unit() << ':' << e.getSecond() << '\n';
  }

  RecognizeIterator(CG);

  // for (const auto &curLoop : *LI) {
  // for (const auto *curBlock : curLoop->getBlocks()) {
  // for (const auto &curInst : *curBlock) {
  // llvm::dbgs() << curInst << '\n';
  //}
  //}
  //}

  return hasChanged;
}

} // namespace itr
