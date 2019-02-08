//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "IteratorRecognition/Support/ShadowDependenceGraph.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "IteratorRecognition/Support/LoopRPO.hpp"

#include "IteratorRecognition/Analysis/CrossIterationDependencyChecker.hpp"

#include "IteratorRecognition/Analysis/DetectOperations.hpp"

#include "IteratorRecognition/Analysis/StaticCommutativityAnalyzer.hpp"

#include "IteratorRecognition/Analysis/IteratorValueTracking.hpp"

#include "IteratorRecognition/Analysis/Passes/PayloadDependenceGraphPass.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionWrapperPass.hpp"

#include "IteratorRecognition/Analysis/ValueClassification.hpp"

#include "Pedigree/Analysis/Passes/PDGraphPass.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/Analysis/AliasAnalysis.h"
// using llvm::AAResultsWrapperPass
// using llvm::AAResults

#include "llvm/IR/Dominators.h"
// using llvm::DominatorTreeWrapperPass
// using llvm::DominatorTree

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/Instructions.h"
// using llvm::StoreInst

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <algorithm>
// using std::reverse

#define DEBUG_TYPE "iterator-recognition-payload-graph"

// namespace aliases

namespace itr = iteratorrecognition;

// plugin registration for opt

char itr::PayloadDependenceGraphPass::ID = 0;
static llvm::RegisterPass<itr::PayloadDependenceGraphPass>
    X("itr-payload-graph",
      PRJ_CMDLINE_DESC(
          "payload dependene graph (based on iterator recognition) pass"),
      false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerPayloadDependenceGraphPass(const llvm::PassManagerBuilder &Builder,
                                   llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::PayloadDependenceGraphPass());

  return;
}

static llvm::RegisterStandardPasses RegisterPayloadDependenceGraphPass(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    registerPayloadDependenceGraphPass);

static llvm::cl::opt<bool> Export("itr-export-updates",
                                  llvm::cl::desc("export graph updates"));

namespace iteratorrecognition {

void PayloadDependenceGraphPass::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::DominatorTreeWrapperPass>();
  AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
  AU.addRequiredTransitive<llvm::AAResultsWrapperPass>();
  AU.addRequiredTransitive<IteratorRecognitionWrapperPass>();

  AU.setPreservesAll();
}

bool PayloadDependenceGraphPass::runOnFunction(llvm::Function &CurFunc) {
  if (FunctionWhiteList.size()) {
    auto found = std::find(FunctionWhiteList.begin(), FunctionWhiteList.end(),
                           std::string{CurFunc.getName()});
    if (found == FunctionWhiteList.end()) {
      return false;
    }
  }

  auto *DT = &getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
  auto &LI = getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  auto &AA = getAnalysis<llvm::AAResultsWrapperPass>().getAAResults();
  auto &IRI = getAnalysis<IteratorRecognitionWrapperPass>()
                  .getIteratorRecognitionInfo();
  unsigned loopCount = 0;

  LLVM_DEBUG({
    llvm::dbgs() << "payload dependence graph for function: "
                 << CurFunc.getName() << "\n";
  });

  llvm::SmallVector<llvm::Loop *, 8> loopTraversal;
  for (auto *e : LI.getLoopsInPreorder()) {
    loopTraversal.push_back(e);
  }
  std::reverse(loopTraversal.begin(), loopTraversal.end());

  for (auto *curLoop : loopTraversal) {
    if (curLoop->getLoopDepth() > LoopDepthMax ||
        curLoop->getLoopDepth() < LoopDepthMin) {
      continue;
    }

    llvm::dbgs() << "#######\n";
    LLVM_DEBUG(llvm::dbgs()
                   << "loop: " << strconv::to_string(*curLoop) << "\n";);

    auto infoOrError = IRI.getIteratorInfoFor(curLoop);
    if (!infoOrError) {
      continue;
    }
    auto &info = *infoOrError;

    llvm::SmallPtrSet<llvm::Instruction *, 8> itVals, pdVals, pdLiveVals;
    llvm::SmallPtrSet<llvm::Instruction *, 8> pdVirtRegLiveVals,
        pdVirtRegLiveInVals, pdVirtRegLiveThruVals, pdVirtRegLiveOutVals;

    FindIteratorValues(info, itVals);
    FindPayloadValues(info, pdVals);
    FindVirtRegPayloadLiveValues(info, pdVals, pdVirtRegLiveVals);
    SplitVirtRegPayloadLiveValues(info, pdVals, pdVirtRegLiveVals, *DT,
                                  pdVirtRegLiveInVals, pdVirtRegLiveThruVals,
                                  pdVirtRegLiveOutVals);

    auto &g = IRI.getGraph();
    llvm::dbgs() << g.size() << '\n';
    using DGType = std::remove_reference_t<decltype(g)>;
    using DGT = llvm::GraphTraits<DGType *>;

    auto is_payload = [&pdVals](const auto *e) {
      return pdVals.count(e->unit()) != 0;
    };

    // step 1 graph update

    auto pdModRefFilter = [&set = pdVals](const llvm::Instruction &I) {
      return I.mayReadOrWriteMemory() && set.count(&I);
    };

    LoopRPO pdModRefTraversal(*curLoop, IRI.getLoopInfo(), pdModRefFilter);

    LLVM_DEBUG({
      llvm::dbgs() << "payload ModRef RPO traversal:\n";
      for (auto *e : pdModRefTraversal) {
        llvm::dbgs() << *e << '\n';
      }
    });

    DependenceCache dc(pdModRefTraversal.begin(), pdModRefTraversal.end(), AA);

    LLVM_DEBUG({
      llvm::dbgs() << "dependence cache:\n";
      dc.print(llvm::dbgs());
    });

    llvm::json::Object jsonInfo;

    IteratorVarianceGraphUpdater<DGType> ivgu(g, dc.begin(), dc.end(), itVals,
                                              *curLoop, &jsonInfo);

    if (Export) {
      WriteJSONToFile(std::move(jsonInfo),
                      "graphupdates." + CurFunc.getName() + ".loop." +
                          std::to_string(loopCount),
                      ".");
    }

    // step 2 create shadow graph

    SDependenceGraph<DGType> sg2(g);
    sg2.computeNodesIf(is_payload);
    sg2.computeEdges();

    // step 3 detect operations

    llvm::SmallVector<LoadModifyStore, 8> lms;
    llvm::SmallPtrSet<llvm::Instruction *, 8> uniqueTargets;

    for (auto *e : pdVirtRegLiveOutVals) {
      lms.emplace_back(LoadModifyStore{e, {}, {}});
    }

    for (auto &e : dc) {
      auto &dep = e.first;
      if (uniqueTargets.count(dep.second)) {
        continue;
      }

      uniqueTargets.insert(dep.second);
      lms.emplace_back(LoadModifyStore{dep.second, {}, {}});
    }

    for (auto &e : lms) {
      DetectOperationsOn(sg2, *curLoop, e);

      llvm::dbgs() << "target: " << *e.Target << '\n';

      llvm::dbgs() << "sources:\n";
      for (auto *s : e.Sources) {
        llvm::dbgs() << *s << '\n';
      }

      llvm::dbgs() << "ops:\n";
      for (auto *o : e.Operations) {
        llvm::dbgs() << *o << '\n';
      }
    }

    // step 4 determine commutativity

    StaticCommutativityAnalyzer sca;
    sca.analyze(lms, itVals, *curLoop);

    loopCount++;
  }

  return false;
} // namespace iteratorrecognition

} // namespace iteratorrecognition
