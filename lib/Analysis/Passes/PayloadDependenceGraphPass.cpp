//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "IteratorRecognition/Support/ShadowDependenceGraph.hpp"

#include "IteratorRecognition/Analysis/ReversePostOrderTraversal.hpp"

#include "IteratorRecognition/Analysis/CrossIterationDependencyChecker.hpp"

#include "IteratorRecognition/Analysis/DetectOperations.hpp"

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

  auto &info = getAnalysis<IteratorRecognitionWrapperPass>()
                   .getIteratorRecognitionInfo();
  auto &AA = getAnalysis<llvm::AAResultsWrapperPass>().getAAResults();
  auto &LI = getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
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

  // for (auto &e : info.getIteratorsInfo()) {
  for (auto *curLoop : loopTraversal) {
    // LLVM_DEBUG(llvm::dbgs() << "loop: " << *e.getLoop()->getHeader() <<
    // "\n";);

    if (curLoop->getLoopDepth() > LoopDepthMax ||
        curLoop->getLoopDepth() < LoopDepthMin) {
      continue;
    }

    llvm::dbgs() << "#######\n";
    LLVM_DEBUG(llvm::dbgs()
                   << "loop: " << curLoop->getHeader()->getName() << "\n";);
    auto infoOrError = info.getIteratorInfoFor(curLoop);

    llvm::SmallPtrSet<llvm::Instruction *, 8> itVals, pdVals, pdLiveVals,
        directItUsesInPayloadVals;

    if (!infoOrError) {
      continue;
    }
    auto &e = *infoOrError;

    FindIteratorValues(e, itVals);
    FindPayloadValues(e, pdVals);
    FindDirectUsesOfIn(itVals, pdVals, directItUsesInPayloadVals);

    auto &g = info.getGraph();
    llvm::dbgs() << g.size() << '\n';
    using DGType = std::remove_reference_t<decltype(g)>;
    using DGT = llvm::GraphTraits<DGType *>;

    auto is_payload = [&pdVals](const auto *e) {
      return pdVals.count(e->unit()) != 0;
    };

    // SDependenceGraph<DGType> sg(g);
    // SDependenceGraph<DGType> sg2(g);
    // sg.computeNodes();
    // sg2.computeNodes();
    // for (const auto &n : DGT::nodes(&g)) {
    // if (!is_payload(n)) {
    // sg.removeNodeFor(n->unit());
    //// sg2.removeNodeFor(n->unit());
    //}
    //}
    // this does not work due to compiler
    // sg.computeNodesIf(is_payload);

    // sg.computeEdges();
    // sg2.computeEdges();

    // llvm::dbgs() << g.size() << '\n';
    // llvm::dbgs() << sg.size() << '\n';
    // llvm::dbgs() << sg.numOutEdges() << '\n';

    // for (auto *n : sg.nodes()) {
    // if (!n->isNextIteration()) {
    // llvm::dbgs() << "node with units: \n";
    // for (auto &u : n->units()) {
    // llvm::dbgs() << "\t" << *u << '\n';
    //}

    // for (auto &e : n->edges()) {
    // llvm::dbgs() << "\thas edge with node with units: \n";
    // for (auto &u : e->units()) {
    // llvm::dbgs() << "\t" << *u << '\n';
    //}
    //}
    //}
    //}

    // sg.computeNextIterationNodes();
    // sg.computeNextIterationEdges();

    // for (auto *n : sg.nodes()) {
    // if (n->isNextIteration()) {
    // llvm::dbgs() << "node with units: \n";
    // for (auto &u : n->units()) {
    // llvm::dbgs() << "\t" << *u << '\n';
    //}

    // for (auto &e : n->edges()) {
    // llvm::dbgs() << "\thas edge with node with units: \n";
    // for (auto &u : e->units()) {
    // llvm::dbgs() << "\t" << *u << '\n';
    //}
    //}
    //}
    //}

    // llvm::dbgs() << sg.size() << '\n';
    // llvm::dbgs() << sg.numOutEdges() << '\n';

    // sg.computeCrossIterationEdges(itVals);

    // llvm::dbgs() << sg.numOutEdges() << '\n';

    //

    // llvm::Instruction *target = nullptr;
    // for (auto *n : sg.nodes()) {
    // for (auto *i : n->units()) {
    // if (llvm::isa<llvm::StoreInst>(i)) {
    // target = i;
    // break;
    //}
    //}

    // if (target) {
    // break;
    //}
    //}

    // step 1 graph update

    ModRefReversePostOrder pdModRefTraversal(*e.getLoop(), info.getLoopInfo(),
                                             pdVals);

    for (auto *e : pdModRefTraversal) {
      llvm::dbgs() << *e << '\n';
    }

    CrossIterationDependencyChecker cidc(pdModRefTraversal.begin(),
                                         pdModRefTraversal.end(), AA);

    // for (auto &dep : cidc) {
    // auto res1 = GetIteratorVariance(dep.first, itVals, e.getLoop());
    // llvm::dbgs() << "I1: " << *dep.first
    //<< " res1: " << static_cast<unsigned>(res1.get()) << '\n';
    // auto res2 = GetIteratorVariance(dep.second, itVals, e.getLoop());
    // llvm::dbgs() << "I2: " << *dep.second
    //<< " res2: " << static_cast<unsigned>(res2.get()) << '\n';
    //}

    llvm::json::Object jsonInfo;

    IteratorVarianceGraphUpdater<DGType> ivgu(g, cidc.begin(), cidc.end(),
                                              itVals, *e.getLoop(), &jsonInfo);

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

    for (auto &dep : cidc) {
      if (uniqueTargets.count(dep.second)) {
        continue;
      }

      uniqueTargets.insert(dep.second);
      lms.emplace_back(LoadModifyStore{dep.second, {}, {}});
    }

    for (auto &e : lms) {
      DetectOperationsOn(sg2, e);

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

    loopCount++;
  }

  return false;
}

} // namespace iteratorrecognition
