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

#include <iterator>
// using std::begin
// using std::end

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

void IteratorRecognitionPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::LoopInfoWrapperPass>();
  AU.addRequired<pedigree::PDGraphPass>();

  AU.setPreservesAll();
}

void MapPDGSCCToLoop(const llvm::LoopInfo &LI, const pedigree::PDGraph &G,
                     ConstCondensationVector &CV,
                     llvm::DenseMap<int, llvm::Loop *> &Map) {
  for (auto i = 0; i < CV.size(); ++i) {
    llvm::Loop *loop = nullptr;

    for (auto j = 0; j < CV[i].size(); ++j) {
      if (!CV[i][j]->unit()) {
        continue;
      }

      loop = LI.getLoopFor((CV[i][j]->unit())->getParent());
      llvm::dbgs() << '-' << (CV[i][j]->unit())->getParent()->getName() << '\n';

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
  ConstCondensationVector CV;

  Graph.connectRootNode();

  llvm::dbgs() << "+++ " << Graph.numOutEdges() << '\n';

  CondensationGraph<CondensationType::value_type> CG;

  for (auto &scc :
       llvm::make_range(llvm::scc_begin(&Graph), llvm::scc_end(&Graph))) {
    CV.emplace_back(scc);
    llvm::dbgs() << "*\n";

    for (auto &e : scc)
      if (e->unit())
        llvm::dbgs() << *e->unit();

    CG.addCondensedNode(std::begin(scc), std::end(scc));
  }

  CG.connectEdges();

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

  llvm::DenseMap<int, llvm::Loop *> PDGSCCToLoop;
  MapPDGSCCToLoop(*LI, Graph, CV, PDGSCCToLoop);

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
