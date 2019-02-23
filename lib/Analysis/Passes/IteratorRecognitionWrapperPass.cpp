//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionWrapperPass.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "IteratorRecognition/Support/Utils/InstTraversal.hpp"

#include "Pedigree/Analysis/Passes/PDGraphPass.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/DenseSet.h"
// using llvm::DenseSet

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "llvm/ADT/Statistic.h"
// using STATISTIC macro

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <iterator>
// using std::distance

#include <system_error>
// using std::error_code

#define DEBUG_TYPE ITR_RECOGNITION_PASS_NAME

// namespace aliases

namespace itr = iteratorrecognition;

STATISTIC(NumTopLevelProcessed, "Number of top-level loops processed");
STATISTIC(NumProcessed, "Number of loops processed");
STATISTIC(NumInseparable, "Number of inseparable top-level loops found");

// plugin registration for opt

char itr::IteratorRecognitionWrapperPass::ID = 0;
static llvm::RegisterPass<itr::IteratorRecognitionWrapperPass>
    X(ITR_RECOGNITION_PASS_NAME, PRJ_CMDLINE_DESC("iterator recognition pass"),
      false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerIteratorRecognitionWrapperPass(const llvm::PassManagerBuilder &Builder,
                                       llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::IteratorRecognitionWrapperPass());

  return;
}

static llvm::RegisterStandardPasses RegisterIteratorRecognitionWrapperPass(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    registerIteratorRecognitionWrapperPass);

//

llvm::AnalysisKey itr::IteratorRecognitionAnalysis::Key;

namespace iteratorrecognition {

// new passmanager pass

IteratorRecognitionAnalysis::Result
IteratorRecognitionAnalysis::run(llvm::Function &F,
                                 llvm::FunctionAnalysisManager &FAM) {
  const auto &LI{FAM.getResult<llvm::LoopAnalysis>(F)};
  pedigree::PDGraph &Graph{*FAM.getResult<pedigree::PDGraphAnalysis>(F)};
  Graph.connectRootNode();

#ifdef LLVM_ENABLE_STATS
  // TODO const_cast is required due to LLVM API inconsistency with constness
  const auto &preorderLoopForest =
      const_cast<llvm::LoopInfo &>(LI).getLoopsInPreorder();
  NumProcessed += preorderLoopForest.size();
  NumTopLevelProcessed += std::distance(LI.begin(), LI.end());
#endif // LLVM_ENABLE_STATS

  // Info = std::make_unique<IteratorRecognitionInfo>(LI, Graph);

  // TODO these stats need to run in the ctor
  //#ifdef LLVM_ENABLE_STATS
  // for (const auto &ii : Info.get()->getIteratorsInfo()) {
  // auto numItInst = ii.getNumInstructions();
  // auto loopInsts = make_loop_inst_range(ii.getLoop());
  // auto numLoopInst = std::distance(loopInsts.begin(), loopInsts.end());

  // if (numLoopInst == numItInst) {
  // NumInseparable++;
  //}
  //}
  //#endif // LLVM_ENABLE_STATS

  // LLVM_DEBUG({
  // for (const auto &ii : Info.get()->getIteratorsInfo()) {
  // auto *hdr = ii.getLoop()->getHeader();
  // llvm::dbgs() << "loop with header: " << hdr << ' ' << hdr->getName()
  //<< '\n';
  // llvm::dbgs() << "\titerator instructions:\n";
  // for (auto *i : ii) {
  // llvm::dbgs() << '\t' << *i << '\n';
  //}
  //}
  //});

  return {LI, Graph};
}

// legacy passmanager pass

void IteratorRecognitionWrapperPass::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
  AU.addRequired<pedigree::PDGraphWrapperPass>();

  AU.setPreservesAll();
}

bool IteratorRecognitionWrapperPass::runOnFunction(llvm::Function &CurFunc) {
  const auto &LI = getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  pedigree::PDGraph &Graph{
      getAnalysis<pedigree::PDGraphWrapperPass>().getGraph()};
  Graph.connectRootNode();

#ifdef LLVM_ENABLE_STATS
  // TODO const_cast is required due to LLVM API inconsistency with constness
  const auto &preorderLoopForest =
      const_cast<llvm::LoopInfo &>(LI).getLoopsInPreorder();
  NumProcessed += preorderLoopForest.size();
  NumTopLevelProcessed += std::distance(LI.begin(), LI.end());
#endif // LLVM_ENABLE_STATS

  Info = std::make_unique<IteratorRecognitionInfo>(LI, Graph);

#ifdef LLVM_ENABLE_STATS
  for (const auto &ii : Info.get()->getIteratorsInfo()) {
    auto numItInst = ii.getNumInstructions();
    auto loopInsts = make_loop_inst_range(ii.getLoop());
    auto numLoopInst = std::distance(loopInsts.begin(), loopInsts.end());

    if (numLoopInst == numItInst) {
      NumInseparable++;
    }
  }
#endif // LLVM_ENABLE_STATS

  LLVM_DEBUG({
    for (const auto &ii : Info.get()->getIteratorsInfo()) {
      auto *hdr = ii.getLoop()->getHeader();
      llvm::dbgs() << "loop with header: " << hdr << ' ' << hdr->getName()
                   << '\n';
      llvm::dbgs() << "\titerator instructions:\n";
      for (auto *i : ii) {
        llvm::dbgs() << '\t' << *i << '\n';
      }
    }
  });

  return false;
}

} // namespace iteratorrecognition
