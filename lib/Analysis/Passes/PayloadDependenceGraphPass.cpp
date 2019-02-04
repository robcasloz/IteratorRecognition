//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Support/ShadowDependenceGraph.hpp"

#include "IteratorRecognition/Analysis/ReversePostOrderTraversal.hpp"

#include "IteratorRecognition/Analysis/DetectOperations.hpp"

#include "IteratorRecognition/Analysis/IteratorValueTracking.hpp"

#include "IteratorRecognition/Analysis/Passes/PayloadDependenceGraphPass.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionWrapperPass.hpp"

#include "IteratorRecognition/Analysis/ValueClassification.hpp"

#include "Pedigree/Analysis/Passes/PDGraphPass.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/Analysis/MemoryLocation.h"

#include "llvm/Analysis/AliasAnalysis.h"

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

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

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

namespace iteratorrecognition {

struct CrossIterationDependencyChecker {
  std::vector<llvm::Instruction *> ModRefInstructions;
  std::map<llvm::Instruction *, llvm::Instruction *> UnresolvedPairs;
  llvm::AAResults &AA;

  template <typename IteratorT>
  CrossIterationDependencyChecker(IteratorT Begin, IteratorT End,
                                  llvm::AAResults &AA)
      : AA(AA) {
    std::copy(Begin, End, std::back_inserter(ModRefInstructions));
  }

  void check() {
    for (auto it1 = ModRefInstructions.begin(), ie1 = ModRefInstructions.end();
         it1 != ie1; ++it1) {
      auto &I1 = *it1;
      for (auto it2 = std::next(it1), ie2 = ie1; it2 != ie2; ++it2) {
        auto &I2 = *it2;
        auto mri = AA.getModRefInfo(I2, llvm::MemoryLocation::getOrNone(I1));
        // llvm::dbgs() << "checking: " << *I1 << "\n" << *I2 << "\n";
        // llvm::dbgs() << "checking: " << static_cast<int>(mri) << "\n";

        if (llvm::isModSet(mri)) {
          llvm::dbgs() << "dep between: " << *I1 << "\n" << *I2 << "\n";
          UnresolvedPairs.insert({I1, I2});
        }
      }
    }
  }
};

void PayloadDependenceGraphPass::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<llvm::AAResultsWrapperPass>();
  AU.addRequiredTransitive<IteratorRecognitionWrapperPass>();

  AU.setPreservesAll();
}

bool PayloadDependenceGraphPass::runOnFunction(llvm::Function &CurFunc) {
  auto &info = getAnalysis<IteratorRecognitionWrapperPass>()
                   .getIteratorRecognitionInfo();
  auto &AA = getAnalysis<llvm::AAResultsWrapperPass>().getAAResults();

  LLVM_DEBUG({
    llvm::dbgs() << "payload dependence graph for function: "
                 << CurFunc.getName() << "\n";
  });

  llvm::SmallPtrSet<llvm::Instruction *, 8> itVals, pdVals, pdLiveVals,
      directItUsesInPayloadVals;

  for (auto &e : info.getIteratorsInfo()) {
    LLVM_DEBUG(llvm::dbgs() << "loop: " << *e.getLoop()->getHeader() << "\n";);

    FindIteratorValues(e, itVals);
    FindPayloadValues(e, pdVals);
    FindDirectUsesOfIn(itVals, pdVals, directItUsesInPayloadVals);

    auto &g = info.getGraph();
    using DGType = std::remove_reference_t<decltype(g)>;
    using DGT = llvm::GraphTraits<DGType *>;

    auto is_payload = [&pdVals](const auto *e) {
      return pdVals.count(e->unit()) != 0;
    };

    SDependenceGraph<DGType> sg(g), sg2(g);
    sg.computeNodes();
    sg2.computeNodes();
    for (const auto &n : DGT::nodes(&g)) {
      if (!is_payload(n)) {
        sg.removeNodeFor(n->unit());
        sg2.removeNodeFor(n->unit());
      }
    }
    // this does not work due to compiler
    // sg.computeNodesIf(is_payload);

    sg.computeEdges();
    sg2.computeEdges();

    llvm::dbgs() << g.size() << '\n';
    llvm::dbgs() << sg.size() << '\n';
    llvm::dbgs() << sg.numOutEdges() << '\n';

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

    sg.computeNextIterationNodes();
    sg.computeNextIterationEdges();

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

    llvm::dbgs() << g.size() << '\n';
    llvm::dbgs() << sg.size() << '\n';
    llvm::dbgs() << sg.numOutEdges() << '\n';

    sg.computeCrossIterationEdges(itVals);

    llvm::dbgs() << sg.numOutEdges() << '\n';

    //

    llvm::Instruction *target = nullptr;
    for (auto *n : sg.nodes()) {
      for (auto *i : n->units()) {
        if (llvm::isa<llvm::StoreInst>(i)) {
          target = i;
          break;
        }
      }

      if (target) {
        break;
      }
    }

    llvm::SmallVector<llvm::Instruction *, 8> operations;

    DetectOperationsOn(target, sg2, operations);

    for (auto *e : operations) {
      llvm::dbgs() << *e << '\n';
    }

    llvm::dbgs() << "#######\n";
    ModRefReversePostOrder pdModRefTraversal(*e.getLoop(), info.getLoopInfo(),
                                             pdVals);

    for (auto *e : pdModRefTraversal) {
      llvm::dbgs() << *e << '\n';
    }

    CrossIterationDependencyChecker cidc(pdModRefTraversal.begin(),
                                         pdModRefTraversal.end(), AA);
    cidc.check();

    // const llvm::DataLayout &DL = CurFunc.getParent()->getDataLayout();
    for (auto &e : cidc.UnresolvedPairs) {
      /*llvm::dbgs() << "underlying obj for: " << *e.first << '\n';
      auto loc1 = llvm::MemoryLocation::getOrNone(e.first);

      if (loc1) {
        const llvm::Value *V1 = llvm::GetUnderlyingObject((*loc1).Ptr, DL);
        llvm::dbgs() << *V1 << '\n';
      }

      llvm::dbgs() << "underlying obj for: " << *e.second << '\n';
      auto loc2 = llvm::MemoryLocation::getOrNone(e.second);

      if (loc2) {
        const llvm::Value *V2 = llvm::GetUnderlyingObject((*loc2).Ptr, DL);
        llvm::dbgs() << *V2 << '\n';
      }
      */

      auto res1 = GetIteratorDependent(e.first, itVals);
      llvm::dbgs() << "I1: " << *e.first
                   << " res1: " << static_cast<unsigned>(res1) << '\n';
      auto res2 = GetIteratorDependent(e.second, itVals);
      llvm::dbgs() << "I2: " << *e.second
                   << " res2: " << static_cast<unsigned>(res2) << '\n';
    }
  }

  return false;
}

} // namespace iteratorrecognition
